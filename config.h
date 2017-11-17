#ifndef CONFIG_H
#define CONFIG_H

void init_config(void);
void config(void);
typedef union {
	long alignment;
	char ag_vt_2[sizeof (int)];
	char ag_vt_4[sizeof (char)];
	char ag_vt_5[sizeof (double)];
} config_vs_type;

typedef enum {
	config_ConfigFile_token = 1, config_lines_token, config_EOF_token,
	config_ws_token, config_line_token = 7, config_EOL_token,
	config_commentchars_token = 10, config_unquotedstring_token = 14,
	config_quotedstring_token = 16, config_signed_integer_token = 22,
	config_unsigned_real_token = 27, config_unsigned_integer_token =
		30,
	config_unquotedstringchar_token =
		32, config_quotedstringchars_token = 34,
	config_RegularChar_token, config_EscapeSequence_token,
	config_DIGIT_token =
		42, config_mantissa_token, config_EXPSYM_token,
	config_integer_part_token, config_fraction_part_token = 47
} config_token_type;

typedef struct {
	config_token_type token_number, reduction_token,
		error_frame_token;
	int input_code;
	int input_value;
	int line, column;
	int ssx, sn, error_frame_ssx;
	int drt, dssx, dsn;
	int ss[32];
	config_vs_type vs[32];
	int bts[32], btsx;
	unsigned char *pointer;
	unsigned char *la_ptr;
	int lab[24], rx, fx;
	const unsigned char *key_sp;
	int save_index, key_state;
	char *error_message;
	char read_flag;
	char exit_flag;
} config_pcb_type;

#ifndef PRULE_CONTEXT
#define PRULE_CONTEXT(pcb)  (&((pcb).cs[(pcb).ssx]))
#define PERROR_CONTEXT(pcb) ((pcb).cs[(pcb).error_frame_ssx])
#define PCONTEXT(pcb)       ((pcb).cs[(pcb).ssx])
#endif

#ifndef AG_RUNNING_CODE_CODE
/* PCB.exit_flag values */
#define AG_RUNNING_CODE         0
#define AG_SUCCESS_CODE         1
#define AG_SYNTAX_ERROR_CODE    2
#define AG_REDUCTION_ERROR_CODE 3
#define AG_STACK_ERROR_CODE     4
#define AG_SEMANTIC_ERROR_CODE  5
#endif

extern config_pcb_type config_pcb;
#endif
