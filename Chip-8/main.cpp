#include "chip.hpp"

//Command line argument #1: Full path to a valid Chip8 binary file
//Command line argument #2 (optional): Clock cycles per second. (The default is 500 if nothing is specified)
int main(int argc, char *argv[]) {
    if(argc < 2){
        std::cout << "No game selected. Use the command line arguments to select a Chip8 binary file from your system." << std::endl;
        return 1;
    }
    
    std::string game_path = argv[1];
    int cycles = 0;
    if(argc >= 3){
        cycles = atoi(argv[2]);
    }

    chip::Chip c(cycles);

    std::string msg = c.run(game_path);
    std::cout << "\n" << msg << std::endl;
    
    return 0;
}
