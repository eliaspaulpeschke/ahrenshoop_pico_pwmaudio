#include <adsr.h>
#include <filter.h>
#include <stdio.h>

#include "audio_pwm.pio.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/rtc.h"
#include "hardware/watchdog.h"
#include "math.h"
#include "notes.h"
#include "pico/stdlib.h"

#define AUDIO_PIN 0
#define AUDIO_BUFFER_LEN 64
#define AUDIO_BUFFER_NUM 3

int dma_chan;
PIO pio;
uint sm;
uint offset;
enum buffer_status {
    empty,
    used,
    full,
    playing
};

volatile static float adc_value;
static float adc_baseline;
static int adc_count;

struct audiobuffer {
    uint32_t buffer[AUDIO_BUFFER_LEN];
    enum buffer_status status;
};

volatile static struct audiobuffer buffers[AUDIO_BUFFER_NUM];
volatile static struct audiobuffer* playing_buffer;
static struct audiobuffer silence;

volatile struct audiobuffer* get_empty_buffer() {
    struct audiobuffer* buf;
    uint8_t it = 0;
    while (1) {
        if (buffers[it].status == empty) {
            return &buffers[it];
        }
        it++;
        if (it >= AUDIO_BUFFER_NUM)
            it = 0;
    }
}

volatile struct audiobuffer* get_playable_buffer() {
    for (uint8_t i = 0; i < AUDIO_BUFFER_NUM; i++) {
        if (buffers[i].status == full) {
            return &buffers[i];
        }
    }
    silence.status = full;
    return &silence;
}

void init_audio_buffers() {
    for (uint i = 0; i < AUDIO_BUFFER_NUM; i++) {
        for (uint j = 0; j < AUDIO_BUFFER_LEN; j++) {
            buffers[i].buffer[j] = 0;
        }
        buffers[i].status = full;
    }
    for (uint i = 0; i < AUDIO_BUFFER_LEN; i++) {
        silence.buffer[i] = 0;
    }
}

uint32_t to_audio(float in) {
    uint32_t value = (uint32_t)(511.0 * (1.0 + in));
    // printf("value %u, ~value %u \n", value, 1024 - value);
    return (((1024 - value) << 10) | value);
}

void init_pio() {
    pio = pio0;
    offset = pio_add_program(pio, &audio_pwm_program);
    sm = pio_claim_unused_sm(pio, true);
    audio_pwm_program_init(pio, sm, offset, AUDIO_PIN);
}

void dma_handler() {
    playing_buffer->status = empty;
    playing_buffer = get_playable_buffer();
    dma_hw->ints0 = 1u << dma_chan;
    dma_channel_set_read_addr(dma_chan, &playing_buffer->buffer, true);
    playing_buffer->status = playing;
}

void init_dma() {
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_PIO0_TX0);

    dma_channel_configure(
        dma_chan,
        &c,
        &pio0_hw->txf[0],  // Write address (only need to set this once)
        NULL,              // Don't provide a read address yet
        AUDIO_BUFFER_LEN,  // Write the same value many times, then halt and interrupt
        false              // Don't start yet
    );

    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    // irq_set_priority(DMA_IRQ_0, 0x00);
}
#define DELTA_THRESH 0.02f
void adc_handler() {
    irq_clear(ADC_IRQ_FIFO);
    static float a = 0.7f;
    static float b = 0.3f;
    if (!adc_fifo_is_empty()) {
        uint16_t val = adc_fifo_get();
        float in = ((((float)val) - 2048.0f) / 2048.0f) - adc_baseline;
        adc_value = in;  //* b + adc_value * a;
    }
}

void adc_make_baseline(){
    adc_baseline = 0.0f;
    float intermediate = 0.0f;
    float delta_intermediate = 0.0f;
    for (int i = 0; i < 10000; i++) {
        sleep_us(500);
        intermediate += adc_value;
    }
    adc_baseline = intermediate / 10000.0f;
    adc_count = 0;
}

void init_adc() {
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);
    adc_fifo_setup(true, false, 1, false, false);
    adc_irq_set_enabled(true);
    adc_set_clkdiv(786);  // 384, 192
    irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_handler);
    irq_set_enabled(ADC_IRQ_FIFO, true);
    adc_run(true);
    adc_make_baseline();
}


#define OSC_BUFFER_LEN 2048
#define OSC_POS_MAX (0x10000 * OSC_BUFFER_LEN)
#define OSC_NUM 16

struct oscillator {
    float (*buffer)[OSC_BUFFER_LEN];
    uint pos;
    uint step;
    uint detune;
    bool is_on;
};

static float sine_buffer[OSC_BUFFER_LEN];
static float saw_buffer[OSC_BUFFER_LEN];
static struct oscillator oscillators[OSC_NUM];

void init_oscillators() {
    for (uint i = 0; i < OSC_BUFFER_LEN; i++) {
        sine_buffer[i] = cosf(2.0f * M_PI * ((float)i / OSC_BUFFER_LEN));
        saw_buffer[i] = -1.0f + 2.0f * ((float)i / OSC_BUFFER_LEN);
    }
    for (int i = 0; i < OSC_NUM; i++) {
        oscillators[i].is_on = false;
    }
}

void start_oscillator(uint num, float (*buffer)[OSC_BUFFER_LEN], uint step, uint detune) {
    if (num < OSC_NUM) {
        oscillators[num].buffer = buffer;
        oscillators[num].step = step;
        oscillators[num].detune = detune;
        oscillators[num].is_on = true;
    }
}

float oscillate(uint num) {
    oscillators[num].pos += oscillators[num].step + oscillators[num].detune;
    if (oscillators[num].pos >= OSC_POS_MAX)
        oscillators[num].pos -= OSC_POS_MAX;
    return (*oscillators[num].buffer)[oscillators[num].pos >> 16u];
}
float clamp_upper(float in, float upper) {
    if (in < upper) return in;
    return upper;
}
float clamp_lower(float in, float lower) {
    if (in > lower) return in;
    return lower;
}
float clamp(float in, float lower, float upper) {
    if (in < lower) return lower;
    if (in > upper) return upper;
    return in;
}

int main() {
    stdio_init_all();
    set_sys_clock_khz(125000, true);
    irq_set_enabled(DMA_IRQ_0, false);
    irq_set_enabled(ADC_IRQ_FIFO, false);
    init_pio();
    init_audio_buffers();
    init_dma();
    dma_handler();
    init_adc();
    init_oscillators();
    rtc_init();
    datetime_t t = {
            .year  = 2020,
            .month = 1,
            .day   = 1,
            .dotw  = 0, // 0 is Sunday, so 5 is Friday
            .hour  = 0,
            .min   = 0,
            .sec   = 0
    };
    rtc_set_datetime(&t);
    sleep_us(128);
    rtc_get_datetime(&t);
    int8_t day_before = t.day; 
    bool first_run = true;
    bool stay_in = true;
    uint8_t delay = 13;
    uint8_t next_mins = 10;
    printf("BOOTED\n");
    while (1){
	    printf("REINIT\n");
	    adc_make_baseline();
	    float vol_1 = 0.1;
	    float vol_2 = 0.1;
	    float vol_all = 0.3;
	    enum notes note_1 = D4;
	    enum notes note_2 = C4;
	    bool waveform = true;
	    start_oscillator(0, &saw_buffer, note_1, 0);
	    start_oscillator(1, &saw_buffer, note_2, 0);
	    start_oscillator(2, &saw_buffer, note_1, 0x4000);
	    start_oscillator(3, &saw_buffer, note_2, 0x8000);
	    struct filter fil;
	    struct filter fil2;
	    set_filter(&fil, 600.0f, 0.4f, true);
	    set_filter(&fil2, 600.0f, 0.4f, true);
            if (t.day > day_before){
		    printf("rebooting!!\n");
		    watchdog_reboot(0, 0, 0);
	    }
	    if (!first_run){
		next_mins = (t.min + delay)%60;
	    }
            stay_in = true;
	    printf("FINISHED INIT\n");

	    while (stay_in == true) {
		rtc_get_datetime(&t);
                if (t.min == next_mins){
		       stay_in = false;	
		       first_run = false;
		}
		volatile struct audiobuffer* buf = get_empty_buffer();
		buf->status = used;
		for (uint i = 0; i < AUDIO_BUFFER_LEN; i++) {
			float for_now = adc_value;
			vol_all = fabsf(for_now);
			float vol = clamp_upper(sqrtf(vol_all), 0.2f);
			float res = clamp(cbrtf(vol_all), 0.3f, 0.84f);
			if (for_now >  0){
			   vol_1 = 0.5 + vol;
			   vol_2 = 0.5 - vol;
			   set_filter(&fil, 600.0f, res, false);
			}else if (for_now < 0){
			   vol_2 = 0.5 + vol;
			   vol_1 = 0.5 - vol;
			   set_filter(&fil, 600.0f, res,  false);
			}
			set_filter(&fil2, 1200.0f, cbrtf(vol_all), false);
			vol_all = clamp(vol_all, 0.05, 0.95);
		        float v = vol_all* (vol_1 * (0.7 * oscillate(0) + 0.3 * oscillate(2)) + vol_2 * (oscillate(1) * 0.7 + oscillate(3)*0.3));
	                buf->buffer[i] = to_audio(filter_audio(&fil, v) * 0.7 + filter_audio(&fil2, v) * 0.3);
		}
		buf->status = full;
	    }
    }
    return 0;
}
