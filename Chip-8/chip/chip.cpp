#include "chip.hpp"

#define PRINT_OPCODES 1

namespace chip{
    Chip::Chip(int clock_hertz){
        if(clock_hertz == 0){
            
            this->clock_hertz = default_clock_hertz;
        }
        else{
            this->clock_hertz = clock_hertz;
        }
    }
    Chip::~Chip(){
    }
    
    std::string Chip::run(std::string path){
        init_cpu();
        init_keyboard();
        
        if(!init_display()){
            return "Error: Display could not be initialized.";
        }
        
        if(!load_game(path)){
            return "Error: ROM could not be loaded.";
        }
        
        int pc = -1;
        bool running = true;
        while(running){
            auto start = std::chrono::steady_clock::now();
            
            
            if(!update_keys()){
                running = false;
                break;
            }
            
            pc = execute_cycle();
            if(pc != -1){
                running = false;
                break;
            }
            
            long us = pow(10, 6) / clock_hertz;
            std::this_thread::sleep_for(std::chrono::microseconds(us));
            
            auto end = std::chrono::steady_clock::now();
            
            elapsed_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            
            long max = (1 / 60.0) * pow(10, 9);
            
            if(elapsed_time > max){
                while(elapsed_time > max){
                    elapsed_time -= max;
                    update_timers();
                }
            }
        }
        
        clean_up_display();
        
        if(pc != -1){
            std::stringstream stream;
            unsigned short opcode = (memory[pc] << 8) | memory[pc + 1];
            stream << "Error: Opcode " << std::hex << opcode << " at memory address " << pc << " is invalid.";
            return stream.str();
        }
        else{
            return "Window terminated by user.";
        }
    }
    
    void Chip::init_cpu(){
        for(int i = 0; i < sizeof(memory); i++){
            memory[i] = 0;
        }
        
        for(int i = 0; i < sizeof(font_set); i++){
            memory[i] = font_set[i];
        }
        
        I = 0x0;
        pc = 0x200;
        stack_ptr = 0;
        delay_timer = 0;
        sound_timer = 0;
        
        for(int i = 0; i < 16; i++){
            v[i] = 0;
            stack[i] = 0;
            keys[i] = 0;
        }
    }
    
    bool Chip::load_game(std::string fileName){
        std::ifstream f(fileName); //taking file as inputstream
        std::string str;
        if(!f) {
            return false;
        }
        
        std::ostringstream ss;
        ss << f.rdbuf(); // reading data
        str = ss.str();
        
        if(str.length() > 0x1000 - 0x200){
            return false;
        }
        
        for(int i = 0; i < str.length(); i++){
            memory[i + 0x200] = str[i];
        }
        
        return true;
    }
    
    int Chip::execute_cycle(){
        //fetch opcode
        const unsigned short opcode = (memory[pc] << 8) | memory[pc + 1]; //each opcode is 2 bytes, so merge two ajdacent spots in memory
        
        //TODO: use unions, since this is the same piece of memory
        const unsigned char instruction = (opcode & 0xF000) >> 12;
        const unsigned short nnn = opcode & 0x0FFF;
        const unsigned char n = opcode & 0x000F;
        const unsigned char x = (opcode & 0x0F00) >> 8;
        const unsigned char y = (opcode & 0x00F0) >> 4;
        const unsigned char kk = opcode & 0x00FF;
        
        std::string to_print;
        switch(instruction){
            case 0x0: {
                switch(nnn){
                    case 0x0E0:{
                        to_print = "Clear the display.";
                        clear_display();
                        break;
                    }
                        
                    case 0x0EE:{
                        to_print = "The interpreter sets the program counter to the address at the top of the stack, then subtracts 1 from the stack pointer.";
                        
                        pc = stack[stack_ptr];
                        stack_ptr--;
                        
                        break;
                    }
                        
                    default:{
                        to_print = "unsupported opcode.";
                        return pc;
                    }
                }
                break;
            }
                
            case 0x1:{
                to_print = "The interpreter sets the program counter to nnn.";
                pc = nnn - 2;
                break;
            }
                
            case 0x2:{
                to_print = "The interpreter increments the stack pointer, then puts the current PC on the top of the stack. The PC is then set to nnn.";
                stack_ptr++;
                stack[stack_ptr] = pc;
                pc = nnn - 2;
                break;
            }
                
            case 0x3:{
                to_print = "The interpreter compares register Vx to kk, and if they are equal, increments the program counter by 2.";
                if(v[x] == kk){
                    pc += 2;
                }
                break;
            }
                
            case 0x4:{
                to_print = "The interpreter compares register Vx to kk, and if they are not equal, increments the program counter by 2.";
                if(v[x] != kk){
                    pc += 2;
                }
                break;
            }
                
            case 0x5:{
                to_print = "The interpreter compares register Vx to register Vy, and if they are equal, increments the program counter by 2.";
                if(v[x] == v[y]){
                    pc += 2;
                }
                break;
            }
                
            case 0x6:{
                to_print = "The interpreter puts the value kk into register Vx.";
                v[x] = kk;
                break;
            }
                
            case 0x7:{
                to_print = "Adds the value kk to the value of register Vx, then stores the result in Vx.";
                v[x] += kk;
                break;
            }
                
            case 0x8:{
                switch(n){
                    case 0x0:{
                        to_print = "Stores the value of register Vy in register Vx.";
                        v[x] = v[y];
                        break;
                    }
                        
                    case 0x1:{
                        to_print = "Performs a bitwise OR on the values of Vx and Vy, then stores the result in Vx. A bitwise OR compares the corrseponding bits from two values, and if either bit is 1, then the same bit in the result is also 1. Otherwise, it is 0.";
                        v[x] |= v[y];
                        break;
                    }
                        
                    case 0x2:{
                        to_print = "Performs a bitwise AND on the values of Vx and Vy, then stores the result in Vx. A bitwise AND compares the corrseponding bits from two values, and if both bits are 1, then the same bit in the result is also 1. Otherwise, it is 0.";
                        v[x] &= v[y];
                        break;
                    }
                        
                    case 0x3:{
                        to_print = "Performs a bitwise exclusive OR on the values of Vx and Vy, then stores the result in Vx. An exclusive OR compares the corrseponding bits from two values, and if the bits are not both the same, then the corresponding bit in the result is set to 1. Otherwise, it is 0.";
                        v[x] ^= v[y];
                        break;
                    }
                        
                    case 0x4:{
                        to_print = "The values of Vx and Vy are added together. If the result is greater than 8 bits (i.e., > 255,) VF is set to 1, otherwise 0. Only the lowest 8 bits of the result are kept, and stored in Vx.";
                        unsigned int ans = v[x] + v[y];
                        if(ans > 0xFF){
                            v[0xF] = 1;
                            //v[x] = ans - 0xFF - 1;
                        }
                        else{
                            v[0xF] = 0;
                            //  v[x] = ans;
                        }
                        
                        v[x] = ans;
                        break;
                    }
                        
                    case 0x5:{
                        to_print = "If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.";
                        if(v[x] > v[y]){
                            v[0xF] = 1;
                            //  v[x] = 0;
                        }
                        else{
                            v[0xF] = 0;
                            //  v[x] -= v[y];
                        }
                        v[x] -= v[y];
                        
                        break;
                    }
                        
                    case 0x6:{
                        to_print = "If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.";
                        if(v[x] % 2 == 1){
                            v[0xF] = 1;
                        }
                        else{
                            v[0xF] = 0;
                        }
                        
                        v[x] /= 2;
                        break;
                    }
                        
                    case 0x7:{
                        to_print = "If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.";
                        if(v[y] > v[x]){
                            v[0xF] = 1;
                        }
                        else{
                            v[0xF] = 0;
                        }
                        v[x] = v[y] - v[x];
                        break;
                    }
                        
                    case 0xE:{
                        to_print = "If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.";
                        if(v[x] >= 0x80){
                            v[0xF] = 1;
                        }
                        else{
                            v[0xF] = 0;
                        }
                        v[x] *= 2;
                        break;
                    }
                        
                    default:{
                        to_print = "unsupported opcode.";
                        return pc;
                    }
                }
                break;
            }
                
            case 0x9:{
                to_print = "The values of Vx and Vy are compared, and if they are not equal, the program counter is increased by 2.";
                if(v[x] != v[y]){
                    pc += 2;
                }
                break;
            }
                
            case 0xA:{
                to_print = "The value of register I is set to nnn.";
                I = nnn;
                break;
            }
                
            case 0xB:{
                to_print = "The program counter is set to nnn plus the value of V0.";
                pc = nnn + v[0] - 2;
                break;
            }
                
            case 0xC:{
                to_print = "The interpreter generates a random number from 0 to 255, which is then ANDed with the value kk. The results are stored in Vx. See instruction 8xy2 for more information on AND.";
                unsigned char rand = std::rand() % 255;
                v[x] = kk & rand;
                break;
            }
                
            case 0xD:{
                to_print = "The interpreter reads n bytes from memory, starting at the address stored in I. These bytes are then displayed as sprites on screen at coordinates (Vx, Vy). Sprites are XORed onto the existing screen. If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0. If the sprite is positioned so part of it is outside the coordinates of the display, it wraps around to the opposite side of the screen. See instruction 8xy3 for more information on XOR, and section 2.4, Display, for more information on the Chip-8 screen and sprites.";
                
                v[0xF] = 0;
                
                const unsigned char s_x = v[x];
                
                for(int i = 0; i < n; i++){
                    unsigned char val = memory[I + i];
                    
                    unsigned char x_coord = s_x;
                    unsigned char y_coord = (v[y] + i) % 32;
                    
                    for(int j = 0; j < 8; j++){
                        bool pixel = val & 0x80;
                        
                        if(display[x_coord + 64 * y_coord] & pixel){
                            v[0xF] = 1;
                        }
                        display[x_coord + 64 * y_coord] ^= pixel;
                        
                        val <<= 1;
                        
                        x_coord = (x_coord + 1) % 64;
                    }
                }
                
                update_display();
                
                break;
            }
                
            case 0xE:{
                switch(kk){
                    case 0x9E:{
                        to_print = "Checks the keyboard, and if the key corresponding to the value of Vx is currently in the down position, PC is increased by 2.";
                        if(keys[v[x]]){
                            pc += 2;
                        }
                        break;
                    }
                        
                    case 0xA1:{
                        to_print = "Checks the keyboard, and if the key corresponding to the value of Vx is currently in the up position, PC is increased by 2.";
                        if(!keys[v[x]]){
                            pc += 2;
                        }
                        break;
                    }
                        
                    default:{
                        to_print = "unsupported opcode.";
                        return pc;
                    }
                }
                break;
            }
                
            case 0xF:{
                switch(kk){
                    case 0x07:{
                        to_print = "The value of DT is placed into Vx.";
                        v[x] = delay_timer;
                        break;
                    }
                        
                    case 0x0A:{
                        to_print = "All execution stops until a key is pressed, then the value of that key is stored in Vx.";
                        bool pressed = false;
                        int i;
                        for(i = 0; i < sizeof(keys); i++){
                            if(keys[i]){
                                pressed = true;
                                break;
                            }
                        }
                        if(!pressed){
                            pc -= 2;
                        }
                        else{
                            v[x] = i;
                        }
                        break;
                    }
                        
                    case 0x15:{
                        to_print = "DT is set equal to the value of Vx.";
                        delay_timer = v[x];
                        break;
                    }
                        
                    case 0x18:{
                        to_print = "ST is set equal to the value of Vx.";
                        sound_timer = v[x];
                        break;
                    }
                        
                    case 0x1E:{
                        to_print = "The values of I and Vx are added, and the results are stored in I.";
                        I += v[x];
                        break;
                    }
                        
                    case 0x29:{
                        to_print = "The value of I is set to the location for the hexadecimal sprite corresponding to the value of Vx. See section 2.4, Display, for more information on the Chip-8 hexadecimal font.";
                        
                        I = v[x] * 5;
                        break;
                    }
                        
                    case 0x33:{
                        to_print = "The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.";
                        
                        memory[I] = v[x] / 100;
                        memory[I + 1] = (v[x] / 10) % 10;
                        memory[I + 2] = v[x] % 10;
                        break;
                    }
                        
                    case 0x55:{
                        to_print = "The interpreter copies the values of registers V0 through Vx into memory, starting at the address in I.";
                        for(int i = 0; i <= x; i++){
                            memory[I + i] = v[i];
                        }
                        break;
                    }
                        
                    case 0x65:{
                        to_print = "The interpreter reads values from memory starting at location I into registers V0 through Vx.";
                        for(int i = 0; i <= x; i++){
                            v[i] = memory[I + i];
                        }
                        break;
                    }
                        
                    default:{
                        to_print = "unsupported opcode.";
                        return pc;
                    }
                }
                break;
            }
                
            default:{
                to_print = "unsupported opcode.";
                return pc;
            }
        }
        
#if PRINT_OPCODES == 1
        std::cout << std::hex << opcode << " " << to_print << std::endl;
#endif
        
        pc += 2;
        return -1;
    }
    
    void Chip::update_timers(){
        if(delay_timer > 0){
            delay_timer--;
        }
        if(sound_timer > 0){
            sound_timer--;
        }
    }
    
    
    bool Chip::init_display(){
        clear_display();
        
        SDL_Init(SDL_INIT_EVERYTHING);
        window = SDL_CreateWindow("Chip8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 64 * pixel_size, 32 * pixel_size, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        
        if(!(window || renderer)){
            return false;
        }
        
        return true;
    }
    
    void Chip::clear_display(){
        for(int i = 0; i < sizeof(display); i++){
            display[i] = false;
        }
    }
    
    bool Chip::update_keys(){
        SDL_Event e;
        if(SDL_PollEvent(&e)){
            if (e.type == SDL_QUIT){
                return false;
            }
            else if(e.type == SDL_KEYDOWN){
                for(int i = 0; i < sizeof(keys); i++){
                    if(e.key.keysym.sym == keycodes[i]){
                        keys[i] = 1;
                    }
                }
            }
            else if(e.type == SDL_KEYUP){
                for(int i = 0; i < sizeof(keys); i++){
                    if(e.key.keysym.sym == keycodes[i]){
                        keys[i] = 0;
                    }
                }
            }
        }
        return true;
    }
    
    void Chip::update_display(){
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        int rows = 32; int cols = 64;
        for(int i = 0; i < rows; i++){
            for(int j = 0; j < cols; j++){
                if(display[j + cols * i]){
                    SDL_Rect rect;
                    rect.x = j * pixel_size;
                    rect.y = i * pixel_size;
                    rect.w = pixel_size;
                    rect.h = pixel_size;
                    
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }
        
        SDL_RenderPresent(renderer);
    }
    
    void Chip::clean_up_display(){
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
    }
    
    void Chip::init_keyboard(){
        for(int i = 0; i < sizeof(keys); i++){
            keys[i] = 0;
        }
    }
}
