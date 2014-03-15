#ifndef __FASTCMP_H__
#define __FASTCMP_H__

// returns false if s != c
// returns true if s == c
FUNCINLINE static ATTRINLINE boolean fasticmp(const char *s, const char *c)
{
	for (; *s && toupper(*s) == toupper(*c); s++, c++) ;
	return (*s == *c); // make sure both strings ended
}

// case-sensitive of the above
FUNCINLINE static ATTRINLINE boolean fastcmp(const char *s, const char *c)
{
	for (; *s && *s == *c; s++, c++) ;
	return (*s == *c); // make sure both strings ended
}

// length-limited of the above
// only true if both strings are at least l characters long AND match, case-sensitively!
FUNCINLINE static ATTRINLINE boolean fastncmp(const char *s, const char *c, UINT16 l)
{
	for (; *s && *s == *c && --l; s++, c++) ;
	return !l; // make sure you reached the end
}

#endif
