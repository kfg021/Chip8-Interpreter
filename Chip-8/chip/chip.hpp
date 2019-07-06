#ifndef CPU_HPP
#define CPU_HPP

#include<iostream>
#include<fstream>
#include<sstream>
#include<SDL2/SDL.h>
#include<thread>
#include<chrono>

namespace chip{
    class Chip{
        
    public:
        Chip(int clock_hertz);
        ~Chip();
        std::string run(std::string game);
        
    private: 
    //cpu stuff
        const int default_clock_hertz = 500;
        int clock_hertz;
        const unsigned char font_set[5 * 16] = {
            0xF0, 0x90, 0x90, 0x90, 0xF0, //0
            0x20, 0x60, 0x20, 0x20, 0x70, //1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
            0x90, 0x90, 0xF0, 0x10, 0x10, //4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
            0xF0, 0x10, 0x20, 0x40, 0x40, //7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
            0xF0, 0x90, 0xF0, 0x90, 0x90, //A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
            0xF0, 0x80, 0x80, 0x80, 0xF0, //C
            0xE0, 0x90, 0x90, 0x90, 0xE0, //D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
            0xF0, 0x80, 0xF0, 0x80, 0x80  //F
        };
        
        unsigned char memory[4096];
        
        unsigned char v[16];
        unsigned short I;
        unsigned short pc;
        
        unsigned int stack[16];
        unsigned char stack_ptr;
        
        unsigned int delay_timer;
        unsigned int sound_timer;
        
        void init_cpu();
        bool load_game(std::string fileName);
        int execute_cycle();
        void update_timers();
        
    // display stuff
        const int pixel_size = 20;
        SDL_Window* window;
        SDL_Renderer* renderer;
        
        bool display[64 * 32];
        
        bool init_display();
        void clear_display();
        void update_display();
        void clean_up_display();
        
    //keyboard stuff
        const SDL_Keycode keycodes[16] = {
            SDLK_x, SDLK_1, SDLK_2, SDLK_3, SDLK_q, SDLK_w, SDLK_e, SDLK_a, SDLK_s, SDLK_d, SDLK_z, SDLK_c, SDLK_4, SDLK_r, SDLK_f, SDLK_v
        };
        
        bool keys[16];
        void init_keyboard();
        
        long elapsed_time = 0;
        
        bool update_keys();
        
    //TODO: implement sound
    };
}

#endif
