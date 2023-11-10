/* Do not remove the following line. Do not remove interrupt_handler(). */
#include "crt0.c" 

#define START	0
#define INGAME	1
#define RESULT	2

#define PIPE_HEIGHT 4
#define PIPE_WIDTH 20
#define PIPE_FPS 6 


/* i/o */
int btn_check_0();
int btn_check_1();
void beep(int mode, int length);
void beep_death();
void beep_congrats();

int check_collision();
void bird_falls();
void bird_flies();

/* util */
unsigned int rand(int cnt);
struct xorshift32_state {
    unsigned int a;
}; 
unsigned int xorshift32(struct xorshift32_state *state);
int get_lcd_pos(int w, int h);

/* display */
void set_score(int score);
void led_set(int data);
void lcd_start();
void lcd_wait(int n);
void lcd_cmd(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_init();
void lcd_result();
void lcd_congrats();
void lcd_congrats_animation();
void lcd_clear();
void lcd_pipe();
void lcd_ingame_update(int pos);
void lcd_customchar(unsigned int addr, unsigned int * bitmap, int pos);
void lcd_printbird(int pos);

int state = START, score, high_score; /* state, score, hscore */
int bird_pos, bird_g_cnt; /* bird rel counter */
int pipe_cnt;
int rand_cnt;
int pipe_pos[PIPE_WIDTH][PIPE_HEIGHT];
int is_jump;
int beep_mode;
int death_sound;
int is_high_score;
int pipe_fps;

/* interrupt_handler() is called every 100msec */
/* handle score and pipe display */
void interrupt_handler()
{
    int random_array[10][4] = {
        {0, 0, 1, 1},
        {1, 1, 1, 0},
        {0, 1, 1, 1},
        {0, 1, 1, 1},
        {1, 1, 0, 0},
        {1, 0, 0, 1},
        {1, 1, 0, 0},
        {1, 1, 1, 0},
        {0, 0, 1, 1},
        {1, 0, 0, 1}
    };
    int i, j;
    if (state == START) {
	    lcd_start();
        set_score(42);
        for (i = 0; i < 20; i++) {
            for (j = 0; j < 4; j++) {
                pipe_pos[i][j] = 0;
            }
        }
    } else if (state == INGAME) {
        rand_cnt++;
        pipe_cnt++;
        bird_g_cnt++;

        if (btn_check_0() && !is_jump) {
            is_jump = 1;
            bird_flies();
        } else if (!btn_check_0() && is_jump) {
            is_jump = 0;
        }

	    if (pipe_cnt % pipe_fps == 0) {
            for (j = 0; j < 4; j++) {
                for (i = 0; i < 19; i++) {
                    pipe_pos[i][j] = pipe_pos[i + 1][j];
                }
                pipe_pos[19][j] = 0;
            }
            if (pipe_cnt % (pipe_fps * 4) == 0) {
                for (j = 0; j < 4; j++) {
                    pipe_pos[19][j] = random_array[rand(bird_g_cnt + score + bird_pos + rand_cnt % 4 + rand_cnt % 21)][j];
                }
                pipe_cnt = 0;
            }
        }
        int fall_cnt = pipe_fps / PIPE_FPS >= 1 ? 5 : 4;

        if (bird_g_cnt != 0 && bird_g_cnt >= fall_cnt) {
            bird_falls();
            bird_g_cnt = 0;
        }

        lcd_ingame_update(bird_pos);

        if (check_collision()) {
            beep_death();
            state = RESULT;
            lcd_clear();
        } else if (pipe_cnt % pipe_fps == 0) {
            int is_score = 0;
            for (j = 0; j < 4; j++) {
                if (pipe_pos[1][j]) {
                    is_score = 1;
                    break;
                }
            }
            if (is_score) {
                score++;
                if (score % 5 == 0 && pipe_fps > 4) {
                    pipe_fps--;
                }
            }
        }
        set_score(score);
        is_high_score = high_score < score;

    } else if (state == RESULT) {
        set_score(score);
    }
}
void main()
{
    state = START;
    score = 0; 
    high_score = 0; 
    bird_pos= 0;
    bird_g_cnt = 0;
    pipe_cnt = 0;
    is_jump = 0;
    beep_mode = 0;
    death_sound = 0;
    is_high_score = 0;
    pipe_fps = PIPE_FPS;
    int i = 0, j = 0;
    int is_wait = 0;
    int congrats_message = 0;
    const unsigned int bitmap[7]=
    {
	    0x00, //00000
	    0x06, //00110
	    0x07, //00111
	    0x1e, //11110
	    0x0c, //01100
	    0x08, //01000
	    0x0c  //01100
    };
    const unsigned int pipe_map[7]=
    {
        0x1f, //11111
        0x1f, //11111
        0x1f, //11111
        0x1f, //11111
        0x1f, //11111
        0x1f, //11111
        0x1f, //11111
    };
    lcd_init();
    lcd_customchar(0x03, bitmap, 0x80);
    lcd_customchar(0x04, pipe_map, 0x80);
    while (1) {
	    if (state == START) {
	        if (btn_check_0()){
		        state = INGAME; 
                lcd_clear();
	        }
            if (btn_check_1()) {
                pipe_fps = 3;
                state = INGAME;
                lcd_clear();
            }
        } else if (state == INGAME) {
        } else if (state == RESULT) {
            if (is_high_score) {
                /* highscore */
                high_score = score;
                if (!congrats_message) {
                    lcd_congrats();
                    if (!death_sound) {
                        death_sound = 1;
                        beep_congrats();
                    }
                    lcd_congrats_animation();
                    congrats_message = 1;
                    lcd_clear();
                }
                lcd_congrats();
            } else {
                /* no highscore */
                lcd_result();
            }
            if (!is_wait) {
                volatile int wait_cnt = 0;
                for (; wait_cnt < 10000000; wait_cnt++) {}
                is_wait = 1;
            } 
            if (btn_check_0() && is_wait) {
                bird_pos= 0;
                bird_g_cnt = 0;
                pipe_cnt = 0;
                score = 0;
                rand_cnt = 0;
                is_wait = 0;
                is_high_score = 0;
                death_sound = 0;
                congrats_message = 0;
                pipe_fps = PIPE_FPS;
                for (i = 0; i < 20; i++) {
                    for (j = 0; j < 4; j++) {
                        pipe_pos[i][j] = 0;
                    }
                }
                state = START;
            }
            if (btn_check_1() && is_wait) {
                bird_pos = 0;
                bird_g_cnt = 0;
                pipe_cnt = 0;
                score = 0;
                rand_cnt = 0;
                is_wait = 0;
                is_high_score = 0;
                death_sound = 0;
                congrats_message = 0;
                pipe_fps = 4;
                for (i = 0; i < 20; i++) {
                    for (j = 0; j < 4; j++) {
                        pipe_pos[i][j] = 0;
                    }
                }
                state = START;
            }
        }
    }
}
/*
 * Switch functions
 */
int btn_check_0()
{
    volatile int *sw_ptr = (int *)0xff04;
    return (*sw_ptr & 0x10) ? 1 : 0;
}
int btn_check_1()
{
    volatile int *sw_ptr = (int *)0xff04;
    return (*sw_ptr & 0x20) ? 1 : 0;
} 
void beep (int mode, int length)
{
    volatile int *ioc_ptr = (int *)0xff14;
    if (mode >= 0 && mode < 15) {
        *ioc_ptr = mode;
        lcd_wait(length);
    }
}
void beep_death()
{ 
    volatile int i = 0;
    int sound_len = 1000000;
    int short_sound_len = 700000;
    int wait_len = 500000;
    int wait_len_long = 750000;
    beep(14, sound_len); // low b
    beep(0, wait_len / 2);
    beep(6, sound_len); // f
    beep(0, wait_len_long);
    beep(6, sound_len); // f
    beep(0, wait_len);
    beep(6, sound_len); // f
    beep(0, wait_len);
    beep(5, short_sound_len); // e
    beep(0, wait_len);
    beep(3, short_sound_len); // d
    beep(0, wait_len);
    beep(1, sound_len * 2); // c
    beep(0, wait_len_long * 2);
}
void beep_congrats()
{
    int sound_len = 1000000;
    int short_sound_len = 700000;
    int wait_len = 500000;
    int wait_len_long = 750000;
    beep(5, sound_len);
    beep(0, wait_len);
    beep(5, sound_len);
    beep(0, wait_len / 2);
    beep(5, sound_len);
    beep(1, sound_len);
    beep(0, wait_len);
    beep(5, sound_len);
    beep(0, wait_len);
    beep(8, sound_len * 3);
    beep(0, wait_len);
}
/*
 * bird
 */
int check_collision()
{
    if (pipe_pos[1][bird_pos] > 0) {
        return 1;
    } else {
        return 0;
    }
}
void bird_falls()
{
    if (bird_pos > 0)
        bird_pos--;
    else
        bird_pos = 0;
}
void bird_flies()
{
    if (bird_pos != 3 && bird_pos >= 0) {
        bird_pos++;
        bird_g_cnt = 0;
    } 
    if (bird_pos == 3) {
        bird_pos = 3;
        bird_g_cnt = 0;
    }
} 
/*
 * Score board functions
 */
void set_score(int score)
{
    volatile int *seg7_ptr = (int *)0xff10;
    *seg7_ptr = score % 100;
}
/*
 * LED functions
 */
void led_set(int data)
{
    volatile int *led_ptr = (int *)0xff08;
    *led_ptr = data;
} 
void lcd_start()
{
    /* GAME START */	
    lcd_cmd(0x84);
    lcd_data(0x47);//G
    lcd_cmd(0x85);
    lcd_data(0x41);//A
    lcd_cmd(0x86);
    lcd_data(0x4d);//M
    lcd_cmd(0x87);
    lcd_data(0x45);//E
    lcd_cmd(0x88);
    lcd_data(0x20);//space
    lcd_cmd(0x89);
    lcd_data(0x53);//S
    lcd_cmd(0x8a);
    lcd_data(0x54);//T
    lcd_cmd(0x8b);
    lcd_data(0x41);//A
    lcd_cmd(0x8c);
    lcd_data(0x52);//R
    lcd_cmd(0x8d);
    lcd_data(0x54);//T
    lcd_printbird(0x95);
} 

void lcd_result()
{
    int score_digit2, score_digit1;
    score_digit2 = (score < 10) ? ' ' : ((score % 100) / 10) + '0';
    score_digit1 = (score % 10) + '0';

    int highscore_digit2, highscore_digit1;
    highscore_digit2 = (high_score < 10) ? ' ' : ((high_score % 100) / 10) + '0';
    highscore_digit1 = (high_score % 10) + '0';

    /*"RESULT:xx"*/	
    lcd_cmd(0xc4);
    lcd_data(0x52);//R
    lcd_cmd(0xc5);
    lcd_data(0x45);//E
    lcd_cmd(0xc6);
    lcd_data(0x53);//S
    lcd_cmd(0xc7);
    lcd_data(0x55);//U
    lcd_cmd(0xc8);
    lcd_data(0x4c);//L
    lcd_cmd(0xc9);
    lcd_data(0x54);//T
    lcd_cmd(0xca);
    lcd_data(0x3a);//:
    lcd_cmd(0xcb);
    lcd_data(score_digit2);//score
    lcd_cmd(0xcc);
    lcd_data(score_digit1);//score

    /*"HIGH SCORE:xx"*/	
    lcd_cmd(0x94);
    lcd_data(0x48);//H
    lcd_cmd(0x95);
    lcd_data(0x49);//I
    lcd_cmd(0x96);
    lcd_data(0x47);//G
    lcd_cmd(0x97);
    lcd_data(0x48);//H
    lcd_cmd(0x98);
    lcd_data(0x20);//space
    lcd_cmd(0x99);
    lcd_data(0x53);//S
    lcd_cmd(0x9a);
    lcd_data(0x43);//C
    lcd_cmd(0x9b);
    lcd_data(0x4f);//O
    lcd_cmd(0x9c);
    lcd_data(0x52);//R
    lcd_cmd(0x9d);
    lcd_data(0x45);//E
    lcd_cmd(0x9e);
    lcd_data(0x3a);//:
    lcd_cmd(0x9f);
    lcd_data(highscore_digit2);//high_score
    lcd_cmd(0xa0);
    lcd_data(highscore_digit1);//high_score
}
void lcd_congrats()
{
    int score_digit2, score_digit1;
    score_digit2 = (score < 10) ? ' ' : ((score % 100) / 10) + '0';
    score_digit1 = (score % 10) + '0';

    int highscore_digit2, highscore_digit1;
    highscore_digit2 = (high_score < 10) ? ' ' : ((high_score % 100) / 10) + '0';
    highscore_digit1 = (high_score % 10) + '0';

    /*"RESULT:xx"*/	
    lcd_cmd(0xc4);
    lcd_data(0x52);//R
    lcd_cmd(0xc5);
    lcd_data(0x45);//E
    lcd_cmd(0xc6);
    lcd_data(0x53);//S
    lcd_cmd(0xc7);
    lcd_data(0x55);//U
    lcd_cmd(0xc8);
    lcd_data(0x4c);//L
    lcd_cmd(0xc9);
    lcd_data(0x54);//T
    lcd_cmd(0xca);
    lcd_data(0x3a);//:
    lcd_cmd(0xcb);
    lcd_data(score_digit2);//score
    lcd_cmd(0xcc);
    lcd_data(score_digit1);//score

    /*"HIGH SCORE:xx"*/	
    lcd_cmd(0x94);
    lcd_data(0x48);//H
    lcd_cmd(0x95);
    lcd_data(0x49);//I
    lcd_cmd(0x96);
    lcd_data(0x47);//G
    lcd_cmd(0x97);
    lcd_data(0x48);//H
    lcd_cmd(0x98);
    lcd_data(0x20);//space
    lcd_cmd(0x99);
    lcd_data(0x53);//S
    lcd_cmd(0x9a);
    lcd_data(0x43);//C
    lcd_cmd(0x9b);
    lcd_data(0x4f);//O
    lcd_cmd(0x9c);
    lcd_data(0x52);//R
    lcd_cmd(0x9d);
    lcd_data(0x45);//E
    lcd_cmd(0x9e);
    lcd_data(0x3a);//:
    lcd_cmd(0x9f);
    lcd_data(highscore_digit2);//high_score
    lcd_cmd(0xa0);
    lcd_data(highscore_digit1);//high_score

    int i, j;
    unsigned char data[9] = {'C', 'O', 'N', 'G', 'R', 'A', 'T', 'S', '!'};
    for (i = 0; i < 9; i++) {
        lcd_cmd(0x84);
        for (j = 0; j < 9; j++) {
            lcd_data(data[j]);
        }
    }   
}
void lcd_congrats_animation()
{
    int score_digit2, score_digit1;
    score_digit2 = (score < 10) ? ' ' : ((score % 100) / 10) + '0';
    score_digit1 = (score % 10) + '0';

    int highscore_digit2, highscore_digit1;
    highscore_digit2 = (high_score < 10) ? ' ' : ((high_score % 100) / 10) + '0';
    highscore_digit1 = (high_score % 10) + '0';

    /*"RESULT:xx"*/	
    lcd_cmd(0xc4);
    lcd_data(0x52);//R
    lcd_cmd(0xc5);
    lcd_data(0x45);//E
    lcd_cmd(0xc6);
    lcd_data(0x53);//S
    lcd_cmd(0xc7);
    lcd_data(0x55);//U
    lcd_cmd(0xc8);
    lcd_data(0x4c);//L
    lcd_cmd(0xc9);
    lcd_data(0x54);//T
    lcd_cmd(0xca);
    lcd_data(0x3a);//:
    lcd_cmd(0xcb);
    lcd_data(score_digit2);//score
    lcd_cmd(0xcc);
    lcd_data(score_digit1);//score

    /*"HIGH SCORE:xx"*/	
    lcd_cmd(0x94);
    lcd_data(0x48);//H
    lcd_cmd(0x95);
    lcd_data(0x49);//I
    lcd_cmd(0x96);
    lcd_data(0x47);//G
    lcd_cmd(0x97);
    lcd_data(0x48);//H
    lcd_cmd(0x98);
    lcd_data(0x20);//space
    lcd_cmd(0x99);
    lcd_data(0x53);//S
    lcd_cmd(0x9a);
    lcd_data(0x43);//C
    lcd_cmd(0x9b);
    lcd_data(0x4f);//O
    lcd_cmd(0x9c);
    lcd_data(0x52);//R
    lcd_cmd(0x9d);
    lcd_data(0x45);//E
    lcd_cmd(0x9e);
    lcd_data(0x3a);//:
    lcd_cmd(0x9f);
    lcd_data(highscore_digit2);//high_score
    lcd_cmd(0xa0);
    lcd_data(highscore_digit1);//high_score

    int i, j;
    unsigned char data[9] = {'C', 'O', 'N', 'G', 'R', 'A', 'T', 'S', '!'};
    for(i=0; i<9; i++){
        lcd_cmd(0x84);
        for(j=0; j<9; j++){
            lcd_data(data[j]);
        }
        for(j=0; j<9-1; j++){
            data[j]=data[j+1];
        }
        data[j] = ' ';
        lcd_wait(2000000);
    }   
}
void lcd_wait(int n)
{
    volatile int i;
    for (i = 0; i < n; i++);
}
void lcd_cmd(unsigned char cmd)
{
    volatile int *lcd_ptr = (int *)0xff0c;
    *lcd_ptr = (0x000000ff & cmd) | 0x00000000;
    lcd_wait(1);
    *lcd_ptr = (0x000000ff & cmd) | 0x00000400;
    lcd_wait(2);
    *lcd_ptr = (0x000000ff & cmd) | 0x00000000;
    lcd_wait(11389);
}
void lcd_data(unsigned char data)
{
    volatile int * lcd_ptr = (int *) 0xff0c;
    *lcd_ptr = (0x000000ff & data) | 0x00000200;
    lcd_wait(1);
    *lcd_ptr = (0x000000ff & data) | 0x00000600;
    lcd_wait(2);
    *lcd_ptr = (0x000000ff & data) | 0x00000200;
    lcd_wait(278);

}
void lcd_init()
{
    lcd_wait(104167);
    lcd_cmd(0x38);
    lcd_cmd(0x06);
    lcd_cmd(0x0c);
    lcd_cmd(0x01);
    lcd_wait(10417);

}
void lcd_customchar(unsigned int addr, unsigned int *bitmap, int pos)
{
    lcd_cmd((addr << 3) | 0x40);
    lcd_data(bitmap[0]);
    lcd_data(bitmap[1]);
    lcd_data(bitmap[2]);
    lcd_data(bitmap[3]);
    lcd_data(bitmap[4]);
    lcd_data(bitmap[5]);
    lcd_data(bitmap[6]);
    lcd_data(0x00);
    lcd_cmd(pos);
} 
void lcd_clear()
{
    lcd_cmd(0x01);
}
void lcd_printbird(int pos)
{
    lcd_cmd(pos);
    lcd_data(0x03);
} 
void lcd_printpipe(int pos)
{
    lcd_cmd(pos);
    lcd_data(0x04);
}
void lcd_ingame_update(int pos) 
{
    // 0x81, 0xc1, 0x95, 0xd5
    int pos_hex = 0;
    switch (pos) {
        case 3:
            pos_hex = 0x81; 
            break;
        case 2:
            pos_hex = 0xc1; 
            break;
        case 1:
            pos_hex = 0x95; 
            break;
        case 0:
            pos_hex = 0xd5; 
            break;
    }
    lcd_clear();
    lcd_pipe();
    lcd_printbird(pos_hex);
}
void lcd_pipe()
{
    int h, w;
    for (w = 0; w < PIPE_WIDTH; w++){
        for(h = 0; h < PIPE_HEIGHT; h++){
            if (pipe_pos[w][h] >= 1) {
                int pos = get_lcd_pos(w, h);
                lcd_printpipe(pos);
            }
        }
    }
} 
/* util */ 
unsigned int xorshift32(struct xorshift32_state *state)
{
    unsigned int x = state->a;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return state->a = x;
} 
unsigned int rand(int cnt)
{
    struct xorshift32_state xorshift32_state;
    xorshift32_state.a = cnt; 
    return xorshift32(&xorshift32_state) % 10;
}
int get_lcd_pos(int w, int h)
{
    switch (h) {
        case 3:
            return 0x80 + w;
            break;
        case 2:
            return 0xc0 + w;
            break;
        case 1:
            return 0x94 + w;
            break;
        case 0:
            return 0xd4 + w;
            break;
        default: 
            return 0x80;
    }
}
