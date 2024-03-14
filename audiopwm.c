#include <stdio.h>
#include <adsr.h>
#include <filter.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "audio_pwm.pio.h"
#include "math.h"



#define AUDIO_PIN 0
#define AUDIO_BUFFER_LEN 256
#define AUDIO_BUFFER_NUM 3
#define BUFFER_LEN 2048
static float sine_buffer[BUFFER_LEN];
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



int main()
{
    stdio_init_all();
    set_sys_clock_khz(125000, true);
    init_pio();
    init_audio_buffers();
    init_dma(); 
  
    for(uint i = 0; i < BUFFER_LEN; i++){
	    //sine_buffer[i] = cosf(2.0f * M_PI * ((float) i / BUFFER_LEN));
	    sine_buffer[i] = -1.0f + 2.0f *((float) i / BUFFER_LEN);
    }
    uint step = 5177344;
    uint pos = 0x80000;
    uint pos2 = 0;
    uint pos3 = 0xF0000;
    uint pos_max = 0x10000 * BUFFER_LEN;
    uint step2 = 3473408 ;
    uint step3 = 4390912;
    bool stepdir = false;
    float vol = 0.4;
    uint volctr = 0;
    struct adsr envelope;
    construct_adsr(&envelope, 0.001f,0.2f,0.8f,0.6f,0.5f);
    struct adsr envelope1;
    construct_adsr(&envelope1, 0.001f,0.2f,0.8f,0.6f,0.7f);
    struct adsr envelope2;
    construct_adsr(&envelope2, 0.6f,0.6f,0.1f,0.05f,0.3f);


    struct filter fil;
    set_filter(&fil, 1000.0f, 0.2f, true);


    dma_handler();
    while(1){
    if (!envelope1.active) envelope1.active = true;  // if(envelope.active) a = 1;
    if (!envelope.active) envelope.active = true;
    if (!envelope2.active) envelope2.active = true;
    //printf("pos: %f, val %f , phase %f, active %d \n", envelope.pos, envelope.value, envelope.phase, a);
	  volatile struct audiobuffer *buf = get_empty_buffer();
	  buf->status = used;
      int c = getchar_timeout_us(0);
      if (c > 0){
        if (c == '['){
          step2 += 0x10000;
        }
        if (c ==']'){
          step2 -= 0x10000;
        }
        printf("%d \r", step2);
      }
	  for (uint i = 0; i < AUDIO_BUFFER_LEN; i++){
      //tick_adsr(&envelope);
      //tick_adsr(&envelope1);
      tick_adsr(&envelope2);
      set_filter(&fil, 600.0f * envelope2.value, 0.6f, false);;

	    float v =  filter_audio(&fil, vol * sine_buffer[pos2 >> 16u]);// + vol * envelope1.value  * sine_buffer[pos >> 16u]); //+ vol * sine_buffer[pos3 >> 16u];
        buf->buffer[i] = to_audio(v);
	    pos += step;
	    pos2 += step2;
        pos3 += step3;
	    if (pos >= pos_max) pos -= pos_max;
	    if (pos2 >= pos_max)pos2 -= pos_max;
	    if (pos3 >= pos_max)pos3 -= pos_max;

      }
	  buf->status = full;
    }
    return 0;
}
