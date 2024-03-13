#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "audio_pwm.pio.h"


#define AUDIO_PIN 0
int main()
{
    stdio_init_all();
    set_sys_clock_khz(125000, true);
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &audio_pwm_program);
    uint sm = pio_claim_unused_sm(pio, true);
    audio_pwm_program_init(pio, sm, offset, AUDIO_PIN);
    //pio_sm_exec(pio, sm, pio_encode_jmp(6));

    while(1){
	  /*  pio_sm_put_blocking(pio, sm, 0xff000000);
	    pio_sm_put_blocking(pio, sm, 0xff000000);
	    pio_sm_put_blocking(pio, sm, 0xff000000);
	    pio_sm_put_blocking(pio, sm, 0xff000000);
	    pio_sm_put_blocking(pio, sm, 0xff000000);
	    pio_sm_put_blocking(pio, sm, 0xff000000);
	    pio_sm_put_blocking(pio, sm, 0xff000000);
	    pio_sm_put_blocking(pio, sm, 0xff000000);
	    pio_sm_put_blocking(pio, sm, 0x00ff0000);
	    pio_sm_put_blocking(pio, sm, 0x00ff0000);
	    pio_sm_put_blocking(pio, sm, 0x00ff0000);
	    pio_sm_put_blocking(pio, sm, 0x00ff0000);
	    pio_sm_put_blocking(pio, sm, 0x00ff0000);
	    pio_sm_put_blocking(pio, sm, 0x00ff0000);
	    pio_sm_put_blocking(pio, sm, 0x00ff0000);
	    pio_sm_put_blocking(pio, sm, 0x00ff0000); */

            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
	
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
            pio_sm_put_blocking(pio, sm, 0x0000ff00);
	    
				    
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    pio_sm_put_blocking(pio, sm, 0x000000ff);
	    
    }



    return 0;
}
