#include <pololu/orangutan.h>
#include <pololu/3pi.h>
#include <stdio.h>


/*
 * simple demo of each sensor
 */


void pause(char *msg, int ms) 
{
    clear();
    printf("wait");
    lcd_goto_xy(0,1);
    printf(msg);
    delay_ms(ms);
}


unsigned long microsecondsSinceStart(unsigned long startTicks)
{
    return ticks_to_microseconds(get_ticks() - startTicks);
}

float secondsSinceStart(unsigned long startTicks)
{
    unsigned long microsecondsElapsed = microsecondsSinceStart(startTicks);
    float microsecondsInSecond=1000000.0; 
    return microsecondsElapsed / microsecondsInSecond;
} 


// Displays the battery voltage.
void batteryVoltage(int seconds)
{
    unsigned long startTicks = get_ticks();
    while (secondsSinceStart(startTicks) < seconds) 
    {
        int bat = read_battery_millivolts();

        clear();
        print_long(bat);
        print("mV");

        delay_ms(100);
    }
}



// counts how many cycles of IR input (high & low) are detected
void irCounter(int seconds)
{

    int loops = 0;
    int res = 0;
    set_digital_input(IO_C5, PULL_UP_ENABLED);

    unsigned char wasHighBefore = 0;

    unsigned long startTicks = get_ticks();
    while (secondsSinceStart(startTicks) < seconds) 
    {
        loops++;
        clear();
        printf("%7ld", loops % 100000);
        lcd_goto_xy(0,1);
        printf("IR:%4d", res%10000);

        unsigned long prevTicks = get_ticks();
        float screenUpdateFreq = 0.1;

        while (secondsSinceStart(prevTicks) < screenUpdateFreq) 
        {

            unsigned char isHighNow = is_digital_input_high(IO_C5);
            if(isHighNow && !wasHighBefore) 
            {
                wasHighBefore = 1;
                res++;
                lcd_goto_xy(0,1);
                printf("IR:%4d", res%10000);
            } 
            else if (!isHighNow && wasHighBefore) 
            {
                wasHighBefore = 0;
            } // else it still has same state as last loop

        }
    }
}

void distanceCheck(int seconds)
{
    // IO_D1 is trigger (output)
    // IO_D0 is echo (input) 

    clear();
    set_digital_input(IO_D0, PULL_UP_ENABLED);
    int loops = 0;

    unsigned long startTicks = get_ticks();
    while (secondsSinceStart(startTicks) < seconds) 
    {
        loops++;

        clear();
        printf("Dist:%2d", loops % 100);

        // send the trigger
        set_digital_output(IO_D1, HIGH);
        delay_us(20);
        set_digital_output(IO_D1, LOW);

        // wait for response to start
        unsigned long prevTicks = get_ticks();
        unsigned char timeout=0;
        while (!is_digital_input_high(IO_D0)) 
        {
            delay_us(5);
            if (secondsSinceStart(prevTicks) > 0.001) {  // should never take more than 0.0002s (200 microseconds)
                clear();
                printf("Dist:%2d", loops);
                lcd_goto_xy(0,1);
                printf("fail 1");
                delay_ms(1000);
                timeout=1;
                break;
            }
        }

        // either timed out or found the start of response
        prevTicks=get_ticks();
        if(!timeout) 
        {
            unsigned long responseLength = 0;
            while (is_digital_input_high(IO_D0)) 
            {
                green_led(TOGGLE);
                red_led(TOGGLE);
                delay_us(5);
                responseLength = microsecondsSinceStart(prevTicks);
                if (responseLength > 100000) {  // 0.1s, max should be 0.038
                    clear();
                    printf("Dist:%2d", loops);
                    lcd_goto_xy(0,1);
                    printf("fail 2");
                    delay_ms(1000);
                    timeout=1;
                    break;
                }
            }

            // either timed out or found the end of the response
            if (!timeout) 
            {
                clear();
                printf("%4d in", (int)(responseLength/148));   // distance (in) = responseLength/148
                lcd_goto_xy(0,1);
                printf("%4d cm", (int)(responseLength/58));    // distance (cm) = responseLength/58
            }
        }

        delay_ms(500);
    }
}


void readAmbientLight(int seconds) 
{
    unsigned long startTicks = get_ticks();
    while (secondsSinceStart(startTicks) < seconds) 
    {
        int loops;

        // ambient light input replaces the user trim potentiometer (which is disabled)
        start_analog_conversion(TRIMPOT);   // start initial conversion
        for (loops=0; loops < 20; )
        {
            if (!analog_is_converting())     // if conversion is done...
            {
                clear();
                int res = analog_conversion_result();
                start_analog_conversion(TRIMPOT);   // start next conversion
                printf("ver1:%2d", loops);
                lcd_goto_xy(0,1);
                printf("%3d/255", res);
                delay_ms(500);
                loops++;
            }
        }

        for (loops=0; loops < 20; loops++)
        {
            clear();
            printf("ver2:%2d", loops);
            lcd_goto_xy(0,1);
            printf("%4u mV", to_millivolts(read_trimpot()));
            delay_ms(500);
        }
    }
}

int main()
{
    lcd_init_printf();
    set_analog_mode(MODE_8_BIT);    // analog-to-digital conversions will give 0-to-255
    pololu_3pi_init_disable_emitter_pin(5000);  // so that PC5 won't be impacted by 3pi line sensor library functions

    while (1) 
    {
        pause("Battery", 500);
        batteryVoltage(5);
        pause("IR", 500);
        irCounter(15);
        pause("Dist", 500);
        distanceCheck(15);
        pause("Light", 500);
        readAmbientLight(40);
    }
}


