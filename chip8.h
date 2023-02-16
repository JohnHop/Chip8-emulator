#if !defined(CHIP8)
#define CHIP8

#include <cstdint>  //per avere gli interi a lunghezza fissa
#include <random>

#define ROM_START_ADDRESS 0x200 //La ROM viene caricata a partire dalla posizione 512 in memoria
#define FONTSET_START_ADDRESS 0x50 //I caratteri invece a partire dalla posizione 80
#define FONTSET_SIZE 80 //Perché sono 16 caratteri di 5 bye ciascuno

#define VIDEO_WIDTH 64
#define VIDEO_HEIGHT 32

/* Rappresenta l'intero sistema e non soltanto il processore virtuale */
class Chip8 {
public:
	Chip8();
	void load_rom(const char* filename);  //Carica la ROM in memoria
	void cycle(); //Esegue il ciclo di Von Neumann / Unità Operativa

  //Dispositivi
	uint8_t keypad[16];
	uint32_t video[VIDEO_WIDTH * VIDEO_HEIGHT]; //bisogna inizializzarlo altrimenti il video esce "sporco"

private:
	//Repertorio delle istruzioni
  void OP_00E0(); // CLS
  void OP_00EE(); // RET
  void OP_1nnn(); // JMP addr
  void OP_2nnn(); // CALL addr
  void OP_3xkk(); // SE Vx, byte
  void OP_4xkk(); // SNE Vx, byte
  void OP_5xy0(); // SE Vx, Vy
  void OP_6xkk(); // LD Vx, byte
  void OP_7xkk(); // ADD Vx, byte
  void OP_8xy0(); // LD Vx, Vy
  void OP_8xy1(); // OR Vx, Vy
  void OP_8xy2(); // AND Vx, Vy
  void OP_8xy3(); // XOR Vx, Vy
  void OP_8xy4(); // ADD Vx, Vy (e setta VF = carry)
  void OP_8xy5(); // SUB Vx, Vy (e setta VF = not borrow)
  void OP_8xy6(); // Vx = Vx >> 1 (e setta VF = LSB)
  void OP_8xy7(); // Vx = Vy - Vx (e setta VF = not borrow)
  void OP_8xyE(); // Vx = vX << 1 (e setta VF = MSB)
  void OP_9xy0(); // SNE Vx, Vy
  void OP_Annn(); // LD I, addr
  void OP_Bnnn(); // JP V0, addr
  void OP_Cxkk(); // RND Vx, byte
  void OP_Dxyn(); // DRW Vx, Vy, nibble (e setta VF = collision)
  void OP_Ex9E(); // SKP Vx
  void OP_ExA1(); // SKNP Vx
  void OP_Fx07(); // LD Vx, DT
  void OP_Fx0A(); // LD Vx, K
  void OP_Fx15(); // LD DT, Vx
  void OP_Fx18(); // LD ST, Vx
  void OP_Fx1E(); // ADD I, Vx
  void OP_Fx29(); // LD F, Vx
  void OP_Fx33(); // LD B, Vx
  void OP_Fx55(); // LD [I], Vx
  void OP_Fx65(); // Ld Vx, [I]

  //Datapath
	uint8_t registers[16];
	uint16_t index;
	uint16_t pc;
	uint16_t stack[16];
	uint8_t sp;
	uint16_t opcode;

  uint8_t memory[4096];
  uint8_t delayTimer;
	uint8_t soundTimer;

  //Utility
	std::default_random_engine randGen;
	std::uniform_int_distribution<uint8_t> randByte;

  //Definizione delle "tabelle" (sono vettori di puntatori a funzioni) per il dispatch dell'opcode
	typedef void (Chip8::*Chip8Fun)();  //definisco un puntatore void (senza argomenti) ad una funzione membro del namespace Chip8
	Chip8Fun table[0xF + 1];  //primo stadio: 16 elementi da 0x0 -> 0xF
	Chip8Fun table0[0xE + 1]; //secondo stadio: l'ultimo digit dell'opcode sarà 0x0 oppure 0xE
	Chip8Fun table8[0xE + 1]; //secondo stadio: l'ultimo digit va da 0x0 -> 0xE
	Chip8Fun tableE[0xE + 1]; //secondo stadio: l'ultimo digit va da 0x1 -> 0xE
	Chip8Fun tableF[0x65 + 1];  //secondo stadio: gli ultimi due digit vanno da 0x07 -> 0x65
  
  //Definizione delle funzioni per il secondo stadio del dispatch

  //Con queste si discrimina in base all'ultimo digit
  void Table0() { (this->*(table0[opcode & 0x000Fu]))(); }
  void Table8() { (this->*(table8[opcode & 0x000Fu]))(); }
  void TableE() { (this->*(tableE[opcode & 0x000Fu]))(); }

  //Qui si discrimina in base agli ultimi due digit
  void TableF() { (this->*(tableF[opcode & 0x00FFu]))(); }

  void OP_NULL() { }
};

inline uint8_t fontset[FONTSET_SIZE] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

#endif // CHIP8