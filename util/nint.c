/* nint.c */

/* the nearest integer of x */
int
nint(double x)
{
    if (x >= 0.0)
	return ((int) (x + 0.5));
    else
	return ((int) (x - 0.5));
}
