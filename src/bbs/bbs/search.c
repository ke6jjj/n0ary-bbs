#include "c_cmmn.h"
#include "search.h"

int front, back, choice;

int
search_start(int cnt)
{
	front = 0;
	back = cnt;
	choice = (back / 2);
	return choice;
}

int
search_next(int direction)
{
	if(direction < 0) {
		front = choice;
		choice = ((back - choice) / 2) + choice;
	} else {
		back = choice;
		choice = ((choice - front) / 2) + front;
	}
	return choice;
}

int
search_range(void)
{
	return (back - front)/2;
}

int
search_end(int direction)
{
	if(direction < 0)
		return choice;
	else
		return front;
}
