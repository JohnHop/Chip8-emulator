#include "chip8.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <random>

Chip8::Chip8()
: video{}, registers{}, index{}, pc{}, stack{}, sp{}, opcode{}, memory{},
  randGen(std::chrono::system_clock::now().time_since_epoch().count()), randByte{0, 255U} //Warning: narrowing conversion
{
	this->pc = ROM_START_ADDRESS; //Il PC ora punta alla prima istruzione della ROM

  std::memcpy(memory + FONTSET_START_ADDRESS, fontset, FONTSET_SIZE); //Carico i caratteri in memoria

	//Inizializzazione delle tabelle per il dispatch dell'opcode
	table[0x0] = &Chip8::Table0;  //passo al secondo stadio
	table[0x1] = &Chip8::OP_1nnn;
	table[0x2] = &Chip8::OP_2nnn;
	table[0x3] = &Chip8::OP_3xkk;
	table[0x4] = &Chip8::OP_4xkk;
	table[0x5] = &Chip8::OP_5xy0;
	table[0x6] = &Chip8::OP_6xkk;
	table[0x7] = &Chip8::OP_7xkk;
	table[0x8] = &Chip8::Table8;  //passo al secondo stadio
	table[0x9] = &Chip8::OP_9xy0;
	table[0xA] = &Chip8::OP_Annn;
	table[0xB] = &Chip8::OP_Bnnn;
	table[0xC] = &Chip8::OP_Cxkk;
	table[0xD] = &Chip8::OP_Dxyn;
	table[0xE] = &Chip8::TableE;  //passo al secondo stadio
	table[0xF] = &Chip8::TableF;  //passo al secondo stadio

  //Inizializzo le tabelle 0, 8 ed E
	for(size_t i = 0; i <= 0xE; ++i) {
		table0[i] = table8[i] = tableE[i] = &Chip8::OP_NULL;
	}

  //Tabella per cui l'opcode inizia per 0x0
	table0[0x0] = &Chip8::OP_00E0;
	table0[0xE] = &Chip8::OP_00EE;

  //Tabella per cui l'opcode inizia per 0x8
	table8[0x0] = &Chip8::OP_8xy0;
	table8[0x1] = &Chip8::OP_8xy1;
	table8[0x2] = &Chip8::OP_8xy2;
	table8[0x3] = &Chip8::OP_8xy3;
	table8[0x4] = &Chip8::OP_8xy4;
	table8[0x5] = &Chip8::OP_8xy5;
	table8[0x6] = &Chip8::OP_8xy6;
	table8[0x7] = &Chip8::OP_8xy7;
	table8[0xE] = &Chip8::OP_8xyE;

  //Tabella per cui l'opcode inizia per 0xE
	tableE[0x1] = &Chip8::OP_ExA1;
	tableE[0xE] = &Chip8::OP_Ex9E;

  //Inizializzo la tabella F
	for(size_t i = 0; i <= 0x65; i++) {
		tableF[i] = &Chip8::OP_NULL;
	}

  //Tabella per cui l'opcode inizia per 0xF
	tableF[0x07] = &Chip8::OP_Fx07;
	tableF[0x0A] = &Chip8::OP_Fx0A;
	tableF[0x15] = &Chip8::OP_Fx15;
	tableF[0x18] = &Chip8::OP_Fx18;
	tableF[0x1E] = &Chip8::OP_Fx1E;
	tableF[0x29] = &Chip8::OP_Fx29;
	tableF[0x33] = &Chip8::OP_Fx33;
	tableF[0x55] = &Chip8::OP_Fx55;
	tableF[0x65] = &Chip8::OP_Fx65;
}

void Chip8::load_rom(const char* filename) {
	std::ifstream file{filename, std::ios::binary | std::ios::ate};

  std::streampos size = file.tellg(); //determino la dimensione del file
  file.seekg(0, std::ios::beg); //ritorno all'inizio del file

  file.read(reinterpret_cast<char*>(memory + ROM_START_ADDRESS), size); //carico il file in memoria a partire dalla locazione 0x200

  //File chiuso automaticamente dal distruttore all'uscita del suo ambito
}

void Chip8::cycle()
{
	//Prelievo
	this->opcode = (memory[pc] << 8u) | memory[pc + 1];
	this->pc += 2;

	//Decodifica ed esecuzione
	(this->*(table[(opcode & 0xF000u) >> 12u]))();

	if(delayTimer > 0) {
		delayTimer -= 1;
	}

	if(soundTimer > 0) {
		soundTimer -= 1;
	}
}

void Chip8::OP_00E0()
{
	memset(video, 0, sizeof(video));
}

void Chip8::OP_00EE() {
  this->sp -= 1;  //ora sp punta alla posizione dello stack appena liberata (ma che contiene ancora il valore del PC salvato)
  this->pc = stack[this->sp];
}

void Chip8::OP_1nnn() {
  this->pc = (opcode & 0x0FFFu);  //l'indirizzo è di 12 bit e salto direttamente ad esso
}

void Chip8::OP_2nnn() {
  stack[sp] = this->pc; //salvo il contesto minimo locale. XXX: pc è uint16_t mentre stack[sp] è uint8_t !!!
  sp += 1;
  pc = opcode & 0xFFFu;
}

//Se Vx == kk allora PC += 2
void Chip8::OP_3xkk() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;  //Shifto di 8 così riporto x a destra
  uint8_t byte = (opcode & 0x00FF);

  if(registers[x] == byte)
    pc += 2;
}

//Se Vx != kk allora PC += 2
void Chip8::OP_4xkk() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;  //Shifto di 8 così riporto x a destra
  uint8_t byte = (opcode & 0x00FF);

  if(registers[x] != byte)
    pc += 2;
}

//Se Vx == Vy allora PC += 2
void Chip8::OP_5xy0() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  if(registers[x] == registers[y])
    pc += 2;
}

void Chip8::OP_6xkk() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t byte = opcode & 0x00FFu;

  registers[x] = byte;
}

void Chip8::OP_7xkk() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t byte = opcode & 0x00FFu;

  registers[x] += byte;
}

void Chip8::OP_8xy0() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  registers[x] = registers[y];
}

void Chip8::OP_8xy1() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  registers[x] |= registers[y];
}

void Chip8::OP_8xy2() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  registers[x] &= registers[y];
}

void Chip8::OP_8xy3() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  registers[x] ^= registers[y];
}

void Chip8::OP_8xy4() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  uint16_t sum = registers[x] + registers[y];

  sum > 255U ? registers[0xF] = 1 : registers[0xF] = 0; //setto VF che è il registro di carry
  registers[x] = sum & 0xFFu; //troncamento
}

void Chip8::OP_8xy5() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  registers[x] > registers[y] ? registers[0xF] = 1 : registers[0xF] = 0; //setto VF che è il registro not borrow

  registers[x] -= registers[y];
}

void Chip8::OP_8xy6() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;

  registers[0x0F] = (registers[x] & 0x1u);  //salvo il LSB in VF

  registers[x] >>= 1;
}

void Chip8::OP_8xy7() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  registers[y] > registers[x] ? registers[0xF] = 1 : registers[0xF] = 0;

  registers[x] = registers[y] - registers[x];
}

void Chip8::OP_8xyE() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;

  registers[0x0F] = (registers[x] & 0x80u) >> 7u;  //salvo il MSB in VF

  registers[x] <<= 1;
}

void Chip8::OP_9xy0() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t y = (opcode & 0x00F0u) >> 4u;

  if(registers[x] != registers[y])
    pc += 2;
}

void Chip8::OP_Annn() {
  index = opcode & 0x0FFFu;
}

//Salta a V0 + nnn
void Chip8::OP_Bnnn() {
  pc = (registers[0] + (opcode & 0x0FFFu));
}

//Vx = random byte AND kk
void Chip8::OP_Cxkk() {
  uint8_t x = (opcode & 0x0F00u) >> 8u;
  uint8_t kk = opcode & 0x00FFu;

  registers[x] = randByte(randGen) & kk;
}

void Chip8::OP_Dxyn() {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	uint8_t height = opcode & 0x000Fu;

	//Evito di andare oltre lo schermo
	uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
	uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

	registers[0xF] = 0; //azzero VF

	for(unsigned int row = 0; row < height; ++row) {
		uint8_t spriteByte = memory[index + row]; //prendo uno sprite (da disegnare orizzontalmente)

		for(unsigned int col = 0; col < 8; ++col) {
			uint8_t spritePixel = spriteByte & (0x80u >> col);  //per ogni bit dello sprite a partire dal MSB
			uint32_t& screenPixel = video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

			if(spritePixel != 0) {  //se il pixel deve essere disegnato
				if(screenPixel == 0xFFFFFFFF) { //collisione
					registers[0xF] = 1;
				}
				screenPixel ^= 0xFFFFFFFF;  //XOR con il pixel già presente sullo schermo
			}
		}
	}
}

void Chip8::OP_Ex9E() {
  uint8_t x = (opcode & 0x0F00u) >> 0x8u;
  uint8_t key = registers[x];

  if(keypad[key])
    pc += 2;
}

void Chip8::OP_ExA1() {
  uint8_t x = (opcode & 0x0F00u) >> 0x8u;
  uint8_t key = registers[x];

  if(!keypad[key])
    pc += 2;
}

void Chip8::OP_Fx07() {
  uint8_t x = (opcode & 0x0F00u) >> 0x8u;

  registers[x] = delayTimer;
}

void Chip8::OP_Fx0A() {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	if (keypad[0]) {
		registers[Vx] = 0;
	}
	else if (keypad[1]) {
		registers[Vx] = 1;
	}
	else if (keypad[2]) {
		registers[Vx] = 2;
	}
	else if (keypad[3]) {
		registers[Vx] = 3;
	}
	else if (keypad[4]) {
		registers[Vx] = 4;
	}
	else if (keypad[5]) {
		registers[Vx] = 5;
	}
	else if (keypad[6]) {
		registers[Vx] = 6;
	}
	else if (keypad[7]) {
		registers[Vx] = 7;
	}
	else if (keypad[8]) {
		registers[Vx] = 8;
	}
	else if (keypad[9]) {
		registers[Vx] = 9;
	}
	else if (keypad[10]) {
		registers[Vx] = 10;
	}
	else if (keypad[11]) {
		registers[Vx] = 11;
	}
	else if (keypad[12]) {
		registers[Vx] = 12;
	}
	else if (keypad[13]) {
		registers[Vx] = 13;
	}
	else if (keypad[14]) {
		registers[Vx] = 14;
	}
	else if (keypad[15]) {
		registers[Vx] = 15;
	}
	else {
		pc -= 2;
	}
}

void Chip8::OP_Fx15() {
  uint8_t x = (opcode & 0x0F00u) >> 0x8u;

  this->delayTimer = registers[x];
}

void Chip8::OP_Fx18() {
  uint8_t x = (opcode & 0x0F00u) >> 0x8u;

  this->soundTimer = registers[x];
}

void Chip8::OP_Fx1E() {
  uint8_t x = (opcode & 0x0F00u) >> 0x8u;

  index += registers[x];
}

void Chip8::OP_Fx29() {
  uint8_t x = (opcode & 0x0F00u) >> 0x8u;
  uint8_t digit = registers[x];

  index = FONTSET_START_ADDRESS + (digit * 5);
}

void Chip8::OP_Fx33() {
  uint8_t x = (opcode & 0x0F00u) >> 0x8u;
  uint8_t value = registers[x];

  //es. 345 / 10 = 34.5 dove value = 34 e in memory[index + 2] = 5
	memory[index + 2] = value % 10;
	value /= 10;

  //quindi 34 / 10 = 3.4 dove value = 3 e memory[index + 1] = 4
	memory[index + 1] = value % 10;
	value /= 10;

  //rimane 3
	memory[index] = value % 10;
}

void Chip8::OP_Fx55() {
  uint8_t x = (opcode & 0x0F00u) >> 0x8u;

  for(uint8_t i = 0; i < x; ++i) {
    memory[index + i] = registers[i];
  }
}

void Chip8::OP_Fx65() {
  uint8_t x = (opcode & 0x0F00u) >> 0x8u;

  for(uint8_t i = 0; i < x; ++i) {
    registers[i] = memory[index + i];
  }
}