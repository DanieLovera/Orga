#include "cache.h"
#include "memory.h"
#include "address_parser.h"
#include <stdlib.h>
#include <stdio.h>

memory_t memory;
cache_t cache;

void _set_init(set_t *self, unsigned int ways_number, unsigned int block_data_size);
void _set_uninit(set_t *self);
void _block_init(block_t *self, unsigned int data_size);
void _block_uninit(block_t *self) ;
unsigned int _cache_total_sets();
unsigned int _cache_block_memory_address(unsigned int tag
	, int setnum, unsigned int total_sets, unsigned int block_size);
int _cache_look_up(int tag, int set);

void cache_init(unsigned int capacity, unsigned int ways_number, unsigned int block_size) {
	cache.capacity = capacity;
	cache.ways_number = ways_number;
	cache.block_size = block_size;
	cache.hits = 0;
	cache.misses = 0;
	unsigned int total_sets = _cache_total_sets();
	cache.sets = (set_t*)malloc(sizeof(set_t) * total_sets);
	for (int set = 0; set < total_sets; set++) {
		_set_init(&(cache.sets[set]), ways_number, block_size);
	}
}

void update_last_used(int set, int way){
	for (int _way = 0; _way < cache.ways_number; _way++){
		cache.sets[set].blocks[_way].last_used = false;
	}
	cache.sets[set].blocks[way].last_used = true;
}

void init() {
	memory_init(&memory);
}

void cache_uninit() {
	unsigned int total_sets = _cache_total_sets();
	for (int set = 0; set < total_sets; set++) {
		_set_uninit(&(cache.sets[set]));
	}
	free(cache.sets);
	memory_uninit(&memory);
}

unsigned int find_set(int address) {
	return address_parser_set(address, cache.block_size, _cache_total_sets());
}

unsigned int find_lru(int setnum) {
	set_t set = cache.sets[setnum];
	int lru_way = 0;
	int min_lru_counter = set.blocks[lru_way].lru_counter;

	for (int way = 0; way < cache.ways_number; way++) {
		// printf("%d\n", set.blocks[way].lru_counter);
		if (set.blocks[way].lru_counter < min_lru_counter) {
			lru_way = way;
			min_lru_counter = set.blocks[lru_way].lru_counter;	
		}
	}

	// printf("via lru %d\n", lru_way);
	return lru_way;
}

unsigned int is_dirty(int way, int setnum) {
	set_t set = cache.sets[setnum];
	block_t block = set.blocks[way];
	return block.dirty;
}

void read_block(int blocknum) { 
	int address = blocknum * cache.block_size;
	unsigned int tag = address_parser_tag(address
		, cache.block_size, _cache_total_sets());
	unsigned int set = address_parser_set(address
		, cache.block_size, _cache_total_sets());
	unsigned int way = find_lru(set);
	
	if (is_dirty(way, set)) {
		write_block(way, set);
	}
	cache.sets[set].blocks[way].valid = true;
	cache.sets[set].blocks[way].dirty = false;
	cache.sets[set].blocks[way].lru_counter = -1;
	cache.sets[set].blocks[way].tag = tag;
	char *data_buffer = cache.sets[set].blocks[way].data;
	memory_read(&memory, data_buffer, address, cache.block_size);
}

void write_block(int way, int setnum) {
	char *data = cache.sets[setnum].blocks[way].data;
	unsigned int tag = cache.sets[setnum].blocks[way].tag;
	unsigned int address = _cache_block_memory_address(tag, setnum
		, _cache_total_sets(), cache.block_size);
	memory_write(&memory, data, address, cache.block_size);
}

char read_byte(int address) {
	unsigned int tag = address_parser_tag(address
		, cache.block_size, _cache_total_sets());
	unsigned int set = address_parser_set(address
		, cache.block_size, _cache_total_sets());
	unsigned int offset = address_parser_offset(address
	, cache.block_size);
	int way = _cache_look_up(tag, set);
	char value = 0;
	if (way >= 0) {
		value = cache.sets[set].blocks[way].data[offset];
		if (!cache.sets[set].blocks[way].last_used){
			cache.sets[set].blocks[way].lru_counter++;
		}
		update_last_used(set, way);
		// printf("Hit en lectura, add: %d \n", address);
	} else {
		// printf("Miss en lectura, add: %d \n", address);
		read_block(address/cache.block_size); 
		value = read_byte(address);
		cache.hits--;
	}
	return value;
}

void write_byte(int address, char value) {
	unsigned int tag = address_parser_tag(address
		, cache.block_size, _cache_total_sets());

	unsigned int set = address_parser_set(address
		, cache.block_size, _cache_total_sets());
	
	unsigned int offset = address_parser_offset(address
	, cache.block_size);
	
	int way = _cache_look_up(tag, set);
	
	if (way >= 0) { // HIT EN ESCRITURA ESCRIBO EN CACHE UNICAMENTE
		cache.sets[set].blocks[way].data[offset] = value;
		cache.sets[set].blocks[way].dirty = true;
		if (!cache.sets[set].blocks[way].last_used){
			cache.sets[set].blocks[way].lru_counter++;
		}
		update_last_used(set, way);
		// printf("Hit en escritura, add: %d, val: %c \n", address, value);
	} else { //  MISS EN ESCRITURA IMPLICA CARGAR EL DATO DE MEMORIA A CACHE Y ESCRIBIR EL DATO
		read_block(address/cache.block_size);
		// printf("Miss en escritura, add: %d, val: %c \n", address, value);
		write_byte(address, value);
		cache.hits--;
	}
}

int get_miss_rate() {
	float total = (float) (cache.misses + cache.hits);
	return ((float) cache.misses / total) * 100;
}

int _cache_look_up(int tag, int set) {
	int _way = -1;
	for (int way = 0; way < cache.ways_number; way++) {
		if (tag == cache.sets[set].blocks[way].tag 
				&& cache.sets[set].blocks[way].valid) {
			_way = way;
			cache.hits++;
			return _way;
		}	
	}
	cache.misses++;
	return _way;
}

unsigned int _cache_block_memory_address(unsigned int tag
	, int setnum, unsigned int total_sets, unsigned int block_size) {
	unsigned int memory_tag_address = tag * 
			(total_sets * block_size);
	unsigned int memory_set_address = setnum * block_size;
	return memory_tag_address + memory_set_address;
}

unsigned int _cache_total_sets() { 
	return cache.capacity / (cache.block_size * cache.ways_number);
}

void _set_init(set_t *self, unsigned int ways_number, unsigned int block_data_size) {
	self->blocks = (block_t *)malloc(sizeof(block_t) * ways_number);
	for (int block = 0; block < ways_number; block++) {
		_block_init(&(self->blocks[block]), block_data_size);
	}
}

void _set_uninit(set_t *self) {
	for (int block = 0; block < cache.ways_number; block++){
		_block_uninit(&(self->blocks[block]));
	}
	free(self->blocks);
}

void _block_init(block_t *self, unsigned int data_size) {
	self->data = (char *)malloc(sizeof(char) * data_size);
	self->tag = 0;
	self->dirty = false;
	self->valid = false;
	self->lru_counter = -1;
	self->last_used = false;
}

void _block_uninit(block_t *self) {
	free(self->data);
	self->tag = 0;
	self->dirty = false;
	self->valid = false;
}