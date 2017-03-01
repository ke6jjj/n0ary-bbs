
void
space_fill(char *out)
{
	int i;
	for(i=0; i<79; i++)
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
	place_string(buf, loc);
}

void
place_hex_number(char *out, int num, int loc)
{
	char buf[25];
	sprintf(buf, "0x%x", num);
	place_string(buf, loc);
}

