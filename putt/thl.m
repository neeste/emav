% thl.m - Show contents of PUTT THL files
function thl(fn,fig)
global f_plt f_pri wd
f_plt = [0 12];         % plot frequency range (kHz)
f_pri = [2 10];         % primary frequency range (kHz)
wd = 0;             		% write data ?
fn = get_fn('*.THL',1);
if (isempty(fn)) return; end;
[f,zl,pl,pr]=load_thl(fn);
show_thl(f,zl,pl,pr,1,fn);
return

% get filename
function fn=get_fn(fn,select)
fn = dir(fn);
if (isempty(fn)) return; end
if (select)
   nf = length(fn);
   if (nf > 1) 
      for k=1:nf
         fprintf('    %d %s\n',k,fn(k).name);
      end
      n  = input(sprintf('\nWhich file (1-%d)? [1] ',nf));
      if (isempty(n))
         n = 1;
      elseif (n<1|n>nf)
         n = 1;
      end
   else
      n = 1;  
   end
   fn = fn(n).name;
end
return

function [f,zl,pl,pr]=load_thl(fn)
fprintf('%s\n',fn);
pr = 0;
if (isoctave)
   load(fn);   		% fetch data
else
   load(fn,'-MAT');	% fetch data
end
n=length(pl);
f=(0:(n-1))*df;
fprintf('%s\n',tsf);
zl(abs(zl)<eps)=eps;
if (length(pr)<length(pl))
   pr = (1 + z_chr ./ zl) / 2;
end
return

function show_thl(f,zl,pl,pr,fig,fn)
global f_plt f_pri wd
figure(fig);clf
fk = f(:)/1000;
ii = find(fk>=f_pri(1) & fk<=f_pri(2));
%
% load impedance
ZL=20*log10(abs(zl)+eps);
ph1=unwrap(angle(zl))/(2*pi);
subplot(2,2,1);
plot(fk,ZL)
axis([f_plt 0 80]);
ylabel('magnitude (dB)')
title('load impedance');
subplot(2,2,3);
plot(fk,ph1)
axis([f_plt -0.55 0.55])
ylabel('phase (cyc)')
xlabel('frequency (kHz)')
text(2,0.35,fn)
%
% load pressure
[nf,nc]=size(pl);
pf=pl .* pr;
PL=20*log10(abs(pl)+eps);
PF=20*log10(abs(pf)+eps);
ph1=unwrap(angle(pl))/(2*pi);
ph2=unwrap(angle(pf))/(2*pi);
gd=-slope(fk(ii),ph1(ii));
ph1=ph1+fk*ones(1,nc)*gd;
ph1=ph1-ones(nf,1)*round(mean(ph1(ii,:)));
ph2=ph2+fk*ones(1,nc)*gd;
ph2=ph2-ones(nf,1)*round(mean(ph2(ii,:)));
s=sprintf('delay=%.2f (ms)',gd);
subplot(2,2,2);
plot(fk,PL,fk,PF)
a=mean(mean([PL PF]));
axis([f_plt a-40 a+40]);
ylabel('magnitude (dB)')
title('load pressure');
subplot(2,2,4);
plot(fk,ph1,fk,ph2)
axis([f_plt -1.04 1.04]);
ylabel('phase (cyc)')
xlabel('frequency (kHz)')
text(2,0.7,s)
if (wd)
   write_data(sprintf('zl%02d.txt',fig),[fj(:) ZL(:) ph1(:)]);
   write_data(sprintf('pl%02d.txt',fig),[fj(:) PL(:) ph1(:)]);
   write_data(sprintf('pf%02d.txt',fig),[fj(:) PF(:) ph2(:)]);
end
return

function slp=slope(x,y)
n=length(x);             % size of both x & y
sx=sum(x);               % sum of x
sy=sum(y);               % sum of y
sxx=dot(x,x);            % sum of x^2
sxy=dot(x,y);            % sum of x*y
slp=(n*sxy-sx*sy)/(n*sxx-sx*sx); % slope
return

function z0 = z_chr
global tc dc
d = tc - 26.85;
r = dc / 2;
c = 3.4723e4 * (1 + 0.00166 * d);
rho = 1.1769e-3 * (1 - 0.00335 * d);
z0 = (rho * c) / (pi * r^2);
return

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function o = isoctave
o=1;eval('OCTAVE_VERSION;','o=0;');

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
function write_data(fn,data)
[nr,nc] = size(data);
fp=fopen(fn,'wt');
fprintf(fp,'; %s\n', fn);
for i=1:nr
   for j=1:nc;
      fprintf(fp,' %14.5g',data(i,j));
   end
   fprintf(fp,'\n');
end
fclose(fp);
return

