import re

def fix_dofft():
    # fix putt/dofft.c
    with open('putt/dofft.c', 'r') as f:
        content = f.read()
    content = content.replace('void gr_rectf();', '')
    with open('putt/dofft.c', 'w') as f:
        f.write(content)
        
    # fix emav/dofft.c
    with open('emav/dofft.c', 'r') as f:
        content = f.read()
    content = content.replace('static void dump_fft(void);', '// static void dump_fft(void);')
    content = content.replace('static void\ndump_fft(void)', '#if 0\nstatic void\ndump_fft(void)')
    content = content.replace('        fprintf(stdout, "\\n");\n    }\n}', '        fprintf(stdout, "\\n");\n    }\n}\n#endif\n')
    with open('emav/dofft.c', 'w') as f:
        f.write(content)

def fix_calfile():
    with open('putt/calfile.c', 'r') as f:
        content = f.read()
    content = content.replace('static int findmax(int    * intbuf, int n);', '// static int findmax(int    * intbuf, int n);')
    content = content.replace('static void tok_init(void);', '// static void tok_init(void);')
    content = content.replace('static void tok_quit(void);', '// static void tok_quit(void);')
    
    # Comment out the function definitions
    content = re.sub(r'(static void\ntok_init\(void\))', r'#if 0\n\1', content)
    content = re.sub(r'(static void\ntok_quit\(void\))', r'#if 0\n\1', content)
    content = re.sub(r'(static int\nfindmax\(int\s+\*\s*intbuf,\s*int\s*n\))', r'#if 0\n\1', content)
    
    # This might be tricky because we need to place #endif at the end of the functions.
    # A simpler way is to just delete the lines, but commenting them out is safer.
    # For now, let's just do a simple replacement if possible.
    with open('putt/calfile.c', 'w') as f:
        f.write(content)
        
def fix_calfile_functions():
    with open('putt/calfile.c', 'r') as f:
        lines = f.readlines()
    out = []
    in_skip = False
    for line in lines:
        if line.startswith('static void\ntok_init(void)') or line.startswith('static void\ntok_quit(void)') or line.startswith('static int\nfindmax(int'):
            in_skip = True
        
        if not in_skip:
            out.append(line)
            
        if in_skip and line.startswith('}'):
            in_skip = False
    with open('putt/calfile.c', 'w') as f:
        f.writelines(out)

def fix_simp():
    for d in ['putt', 'emav']:
        with open(f'{d}/simp.c', 'r') as f:
            content = f.read()
        
        # Replace K&R definition of simpfit
        kr_def = """simpfit(iniv, npv, mxiter, mniter, pvar, prep, peex)
float   iniv[];
int     npv, mxiter, mniter;
double  (*pvar) ();
void    (*prep) ();
int     (*peex) ();
{"""
        ansi_def = """void simpfit(float *iniv, int npv, int mxiter, int mniter, double (*pvar)(float *), void (*prep)(float *), int (*peex)(void))
{"""
        content = content.replace(kr_def, ansi_def)
        with open(f'{d}/simp.c', 'w') as f:
            f.write(content)

def fix_prototypes():
    # putt.c
    with open('putt/putt.c', 'r') as f:
        content = f.read()
    if 'void Quit_wind(int);' not in content:
        content = content.replace('#include "menu.h"', '#include "menu.h"\nvoid Quit_wind(int);')
    with open('putt/putt.c', 'w') as f:
        f.write(content)
        
    # hear_w.c
    with open('putt/hear_w.c', 'r') as f:
        content = f.read()
    if 'float get_first_lev' not in content:
        content = content.replace('#include "menu.h"', '#include "menu.h"\nfloat get_first_lev(float start, float step);\nfloat get_next_lev(float cur, int yn);')
    with open('putt/hear_w.c', 'w') as f:
        f.write(content)
        
    # mlmeer.c
    with open('putt/mlmeer.c', 'r') as f:
        content = f.read()
    if 'void simpfit' not in content:
        content = content.replace('#include "menu.h"', '#include "menu.h"\nvoid simpfit(float *iniv, int npv, int mxiter, int mniter, double (*pvar)(float *), void (*prep)(float *), int (*peex)(void));')
    with open('putt/mlmeer.c', 'w') as f:
        f.write(content)
        
    # probe_w.c
    with open('putt/probe_w.c', 'r') as f:
        content = f.read()
    if 'void simpfit' not in content:
        content = content.replace('#include "menu.h"', '#include "menu.h"\nvoid simpfit(float *iniv, int npv, int mxiter, int mniter, double (*pvar)(float *), void (*prep)(float *), int (*peex)(void));')
    with open('putt/probe_w.c', 'w') as f:
        f.write(content)


fix_dofft()
fix_calfile()
fix_calfile_functions()
fix_simp()
fix_prototypes()
