#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>

#define MAX_FUNCTIONS_TO_RUN 2
#define STDIN_PARAM_IDENTIFIER "-"
#define MIN_VALUE_INPUT 2
#define MAX_VALUE_INPUT INT_MAX

extern unsigned int mcd_euclides(unsigned int a, unsigned int b);
extern unsigned int mcm_euclides(unsigned int a, unsigned int b);

typedef unsigned int (*bin_operation_t) (unsigned int, unsigned int);

void show_usage() {
	printf("Usage:\n"
		   "	common -h\n"
		   "	common -V\n"
		   "	common [options] M N\n"
		   "Options:\n"
		   "	-h, --help Prints usage information.\n"
		   "	-V, --version Prints version information.\n"
		   "	-o, --output Path to output file.\n"
		   "	-d --divisor Just the divisor\n"
		   "	-m --multiple Just the multiple\n");
}

void show_version() {
	printf("Version 1.0\n");
}

int parse_argv(int argc, char *argv[], FILE* output_file, 
			   bin_operation_t functions[], int *functions_to_run) {
	opterr = 0; // getopt suprime los mensajes de error
	static struct option argument_options[] = {
		{"help", no_argument, 0, 'h'},
		{"version", no_argument, 0, 'V'},
		{"output", required_argument, 0, 'o'},
		{"divisor", no_argument, 0, 'd'},
		{"multiple", no_argument, 0, 'm'},
		{0, 0, 0, 0} // Lo pide getopt
	};

	int opt;
	int option_index = 0;
	int last_index = 1;
	while ((opt = getopt_long(argc, argv, "hVo:dm", argument_options, 
														&option_index)) != -1) {
		bool should_update_index = true;
		switch (opt) {
		case 'h':
			show_usage();
			exit(EXIT_SUCCESS);
		case 'V':
			show_version();
			exit(EXIT_SUCCESS);
		case 'o':
			if (strcmp(optarg, STDIN_PARAM_IDENTIFIER) != 0) {
				output_file = fopen(optarg, "w");
				if (!output_file) {
					fprintf(stderr, "Error: No se pudo abrir el archivo %s\n", optarg);
					exit(EXIT_FAILURE);
				}
			}
			last_index++;
			break;
		case 'd':
			functions[(*functions_to_run)++] = mcd_euclides;
			break;
		case 'm':
			functions[(*functions_to_run)++] = mcm_euclides;
			break;
		default:
			should_update_index = false;
			break;
		}
		option_index = 0;
		if (should_update_index)
			last_index++;
	}
	return last_index;
}

bool is_in_range(unsigned int value, unsigned int min, unsigned int max) {
	return min <= value && value < max;
}

bool is_a_number(char* num) {
	if (num[0] != '-' && !isdigit(num[0])) 
		return false;

	size_t len_number = strlen(num);
	for (size_t i = 1; i < len_number; i++){
		if (!isdigit(num[i])) 
			return false;
	}
	return true;
}

bool extract_numbers_parameters(char *str_a, char *str_b, 
											unsigned int *a, unsigned int *b) {
	if (!str_a || !str_b) {
		fprintf(stderr, "Error: Faltan los números.\n");
		return false;
	}

	if (!is_a_number(str_a) || !is_a_number(str_b)) {
		fprintf(stderr, "Error: deben ingresarse numeros no cadenas de texto\n");
		return false;
	}

	*a = atoi(str_a);
	*b = atoi(str_b);

	if (!is_in_range(*a, MIN_VALUE_INPUT, MAX_VALUE_INPUT) || 
		!is_in_range(*b, MIN_VALUE_INPUT, MAX_VALUE_INPUT)) {
		fprintf(stderr, "Error: Número fuera de rango [%d, %d).\n", 
											MIN_VALUE_INPUT, MAX_VALUE_INPUT);
		return false;
	}
	return true;
}

int main(int argc, char *argv[]) {
	FILE* output_file = stdout;
	bin_operation_t functions[] = {
		mcd_euclides,
		mcm_euclides
	};
	int functions_to_run = 0;
	
	int last_index = parse_argv(argc, argv, output_file, functions, &functions_to_run);

	if (!output_file) {
		fprintf(stderr, "Error: No se pudo acceder al archivo de salida.\n");
		exit(EXIT_FAILURE);
	}
	
	unsigned int a;
	unsigned int b;
	if (!extract_numbers_parameters(argv[last_index], argv[last_index + 1], 
																	&a, &b)) {		
		if (output_file != stdout)
			fclose(output_file);
		exit(EXIT_FAILURE);
	}
	
	if (functions_to_run == 0)
		functions_to_run = MAX_FUNCTIONS_TO_RUN;

	for (int i = 0; i < functions_to_run; i++)
		fprintf(output_file, "%u\n", functions[i](a, b));

	if (output_file != stdout)
		fclose(output_file);

	return EXIT_SUCCESS;
}
