#include <stdio.h>
#include <adsr.h>
#include <filter.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "audio_pwm.pio.h"
#include "math.h"
#include "notes.h"

#define AUDIO_PIN 0
#define AUDIO_BUFFER_LEN 256
#define AUDIO_BUFFER_NUM 3

int dma_chan;
PIO pio;
uint sm;
uint offset;
enum buffer_status {empty, used, full, playing};

struct audiobuffer {
   uint32_t buffer[AUDIO_BUFFER_LEN];
   enum buffer_status status;
};

volatile static struct audiobuffer buffers[AUDIO_BUFFER_NUM];
volatile static struct audiobuffer *playing_buffer;
static struct audiobuffer silence; 

volatile struct audiobuffer *get_empty_buffer(){
	struct audiobuffer *buf;
	uint8_t it = 0;
	while (1){
		if (buffers[it].status == empty){
			return &buffers[it];
		}
		it++;
		if (it >= AUDIO_BUFFER_NUM) it = 0;
	}
}

volatile struct audiobuffer *get_playable_buffer(){
	for(uint8_t i = 0; i < AUDIO_BUFFER_NUM; i++){
	    if(buffers[i].status == full){
		    return &buffers[i];
	    }
	}
	silence.status = full;
	return &silence;
}

void init_audio_buffers(){
	for (uint i = 0; i < AUDIO_BUFFER_NUM; i++){
		for (uint j = 0; j < AUDIO_BUFFER_LEN; j++){
			buffers[i].buffer[j] = 0;
		}
		buffers[i].status = full;
	}
	for(uint i = 0; i < AUDIO_BUFFER_LEN; i++){
		silence.buffer[i] = 0;
	}

}

uint32_t to_audio(float in){
	uint32_t value = (uint32_t) (511.0 * (1.0 + in));
	//printf("value %u, ~value %u \n", value, 1024 - value);
	return (((1024 - value) << 10) | value);
}


void init_pio(){
    pio = pio0;
    offset = pio_add_program(pio, &audio_pwm_program);
    sm = pio_claim_unused_sm(pio, true);
    audio_pwm_program_init(pio, sm, offset, AUDIO_PIN);
}

void dma_handler(){
    playing_buffer->status = empty;
    playing_buffer = get_playable_buffer();
    dma_hw->ints0 = 1u << dma_chan;
    dma_channel_set_read_addr(dma_chan, &playing_buffer->buffer, true);
    playing_buffer->status = playing;
}

void init_dma(){
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_PIO0_TX0);

    dma_channel_configure(
        dma_chan,
        &c,
        &pio0_hw->txf[0], // Write address (only need to set this once)
        NULL,             // Don't provide a read address yet
        AUDIO_BUFFER_LEN, // Write the same value many times, then halt and interrupt
        false             // Don't start yet
    );

    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

#define OSC_BUFFER_LEN 2048
#define OSC_POS_MAX (0x10000 * OSC_BUFFER_LEN)
#define OSC_NUM 16

struct oscillator {
    float (*buffer)[OSC_BUFFER_LEN];
    uint pos;
    uint step;
    bool is_on;
};

static float sine_buffer[OSC_BUFFER_LEN];
static float saw_buffer[OSC_BUFFER_LEN];
static struct oscillator oscillators[OSC_NUM]; 

void init_oscillators(){
  for(uint i = 0; i < OSC_BUFFER_LEN; i++){
	    sine_buffer[i] = cosf(2.0f * M_PI * ((float) i / OSC_BUFFER_LEN));
	    saw_buffer[i] = -1.0f + 2.0f *((float) i / OSC_BUFFER_LEN);
    }
  for(int i = 0; i < OSC_NUM; i++){
    oscillators[i].is_on = false;
  }
}

void start_oscillator(uint num, float (*buffer)[OSC_BUFFER_LEN], uint step){
  if (num < OSC_NUM){
    oscillators[num].buffer = buffer;
    oscillators[num].step = step;
    oscillators[num].is_on = true;
  }
}

float oscillate(uint num){
  oscillators[num].pos += oscillators[num].step;
  if (oscillators[num].pos >= OSC_POS_MAX) oscillators[num].pos -= OSC_POS_MAX;
  return (*oscillators[num].buffer)[oscillators[num].pos >> 16u];
}


int main()
{
    stdio_init_all();
    set_sys_clock_khz(125000, true);
    init_pio();
    init_audio_buffers();
    init_dma(); 
    init_oscillators();
    
    //start_oscillator(0, &sine_buffer, 5177344);
    //start_oscillator(1, &saw_buffer, 3473408);
    //start_oscillator(2, &saw_buffer, 4390912);
    
    float vol = 0.4;
    
    /*struct adsr envelope;
    construct_adsr(&envelope, 0.001f,0.2f,0.8f,0.6f,0.5f);
    struct adsr envelope1;
    construct_adsr(&envelope1, 0.001f,0.2f,0.8f,0.6f,0.7f);
    struct adsr envelope2;
    construct_adsr(&envelope2, 0.6f,0.6f,0.1f,0.05f,0.3f);
*/

    struct adsr envs[12];
    for (int i = 0; i < 12; i++){
      construct_adsr(&envs[i], 0.001f,0.02f,0.4f,0.6f,0.7f);
      start_oscillator(i, &saw_buffer, notes[i]);
    }

    struct adsr filter_env;
    construct_adsr(&filter_env, 0.02f, 0.3f, 0.6f, 0.3f, 0.7f);
    struct filter fil;
    set_filter(&fil, 600.0f, 0.8f, true);
    
    char chars[] = {'a', 'w', 's', 'e', 'd', 'f','t', 'g', 'z', 'h', 'u', 'j', 'k'};
    uint mul = 0;
    dma_handler();
    while(1){
/*    if (!envelope1.active) envelope1.active = true;  // if(envelope.active) a = 1;
    if (!envelope.active) envelope.active = true;
    if (!envelope2.active) envelope2.active = true;
  */  //printf("pos: %f, val %f , phase %f, active %d \n", envelope.pos, envelope.value, envelope.phase, a);
	  volatile struct audiobuffer *buf = get_empty_buffer();
	  buf->status = used;
      int c = getchar_timeout_us(0);
      if (c > 0){ 
        if (c == 'n') mul += 1;
        if (c == 'b') mul -= 1;
        if(mul > 3 || mul < 0) mul = 0;
        for (int i = 0; i < 12; i++){
          if( c =='b' || c== 'n'){
            start_oscillator(i, &saw_buffer, notes[i + 12 * mul]);
          }else{
              if (c == chars[i]) {
                restart_envelope(&envs[i]);
                restart_envelope(&filter_env);
                printf("%c, %f \n", c, vol);
              }
          }
        }
        if (c == '+') vol += 0.1;
        if (c == '-') vol -= 0.1;
        if(vol > 1.0f || vol < 0.0f) vol = 0.1f;
      }
	  for (uint i = 0; i < AUDIO_BUFFER_LEN; i++){
      //tick_adsr(&envelope);
      //tick_adsr(&envelope1);
      //tick_adsr(&envelope2);
      //set_filter(&fil, 600.0f * envelope2.value, 0.6f, false);;
      tick_adsr(&filter_env);
      set_filter(&fil, 500.0f * filter_env.value, 0.8f, false);
	    float v = 0;//filter_audio(&fil, vol * oscillate(0));// + vol * envelope1.value  * sine_buffer[pos >> 16u]); //+ vol * sine_buffer[pos3 >> 16u];
      for (int i = 0; i < 12; i++){
         tick_adsr(&envs[i]);
         v += vol * envs[i].value * oscillate(i);
      }
      buf->buffer[i] = to_audio(filter_audio(&fil, v));
     }
	   buf->status = full;
    }
    return 0;
}
