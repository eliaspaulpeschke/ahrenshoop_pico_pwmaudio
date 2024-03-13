#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "audio_pwm.pio.h"
#include "math.h"


#define AUDIO_PIN 0
#define BUFFER_LEN 2048
static float sine_buffer[BUFFER_LEN];

uint32_t to_audio(float in){
	uint32_t value = (uint32_t) (127.0 * (1.0 + in)) >> 1;
	return ((0x000000ff & (255 - value)) << 8 | value);
}



int main()
{
    stdio_init_all();
    set_sys_clock_khz(125000, true);
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &audio_pwm_program);
    uint sm = pio_claim_unused_sm(pio, true);
    audio_pwm_program_init(pio, sm, offset, AUDIO_PIN);
    //sleep_ms(3000);
    printf("init\n");
    //pio_sm_exec(pio, sm, pio_encode_jmp(6));
    for(uint i = 0; i < BUFFER_LEN; i++){
	sine_buffer[i] = cosf(2.0f * M_PI * ((float) i / BUFFER_LEN));
    }
    uint step = 0x200000;
    uint pos = 0;
    uint pos2 = 0;
    uint pos_max = 0x10000 * BUFFER_LEN;
    uint step2 = 0x300000;
    bool stepdir = false;
    float vol = 0.5;
    uint volctr = 0;
    while(1){
	float v = (vol * sine_buffer[pos >> 16u]  + (1.0f - vol) * sine_buffer[pos2 >> 16u]);
        pio_sm_put_blocking(pio, sm, to_audio(v));
	pos += step;
	pos2 += step2;
	if (pos >= pos_max) {
		pos -= pos_max;
		}
	if (pos2 >= pos_max) pos2 -= pos_max;
	volctr += 1;
	if (volctr == 512){
	    volctr = 0;
	    if (stepdir){
	        vol += 0.1f;
	    }else{
		vol -= 0.2f;
	    }
	   if (vol >= 1.0f || vol <= 0.0f) stepdir = !stepdir;
	}

    }



    return 0;
}
