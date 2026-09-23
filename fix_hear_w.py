import re

with open('putt/hear_w.c', 'r') as f:
    content = f.read()

# Replace my wrong float prototypes
content = content.replace('float get_first_lev(float start, float step);\nfloat get_next_lev(float cur, int yn);', '')
content = content.replace('float get_first_lev(float start, float step);\n', '')
content = content.replace('float get_next_lev(float cur, int yn);\n', '')

# Replace old double prototypes with arguments
content = content.replace('double  get_first_lev();', 'double  get_first_lev(double, double);')
content = content.replace('double  get_next_lev();', 'double  get_next_lev(double, int);')
content = content.replace('double  pick_ml_lev();', 'double  pick_ml_lev(void);')
content = content.replace('double  cmp_sd_lev();', 'double  cmp_sd_lev(void);')

with open('putt/hear_w.c', 'w') as f:
    f.write(content)
