#include <stdio.h>

#include "c_cmmn.h"
#include "tools.h"


void
space_fill(char *out, int len)
{
	int i;
	for(i=0; i<len; i++)
		out[i] = ' ';
	out[i] = 0;
}

void
place_string(char *out, char *s, int loc)
{
	char *p = &out[loc];
	while(*s)
		*p++ = *s++;
}

void
place_number(char *out, int num, int loc)
{
	char buf[25];
	sprintf(buf, "%d", num);
	place_string(out, buf, loc);
}

void
place_hex_number(char *out, int num, int loc)
{
	char buf[25];
	sprintf(buf, "0x%x", num);
	place_string(out, buf, loc);
}

void
disp_column_wise(struct text_line **output,
	struct text_line *headers, struct text_line *list)
{
	struct text_line *tl;
	int max_length = textline_maxlength(list);
	int cnt = textline_count(list);
	int columns = 80 / (max_length + 5);
	int rows = cnt / columns;
	int i;
	char tmp[128];

	if(cnt % columns) rows++;

	space_fill(tmp, 79);
	for(i=0; i<rows; i++)
		textline_append(output, tmp);
	textline_append(output, tmp);
	textline_append(output, tmp);

	tl = list;
	for(i=0; i<columns; i++) {
		int position = i * (max_length + 3);
		struct text_line *row = *output;
		struct text_line *head = headers;

		while(head) {
			place_string(row->s, head->s, position);
			NEXT(row);
			NEXT(head);
		}

		while(row && tl) {
			place_string(row->s, tl->s, position);	
			NEXT(row);
			NEXT(tl);
		}
		if(tl == NULL)
			break;
	}
}
