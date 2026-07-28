% thevld - calculate load impedance from CAL & THS files
function thevld
global f_plt f_pri
f_plt = [0 12];         % plot frequency range (kHz)
f_pri = [2 10];         % primary frequency range (kHz)
%
cfn = get_fn('*.CAL',1);
if (isempty(cfn)) return; end;
[f,pl,rate]=load_cal(cfn);
sfn = get_fn('*.THS',1);
[f,zs,ps] = load_ths(sfn);
if (isempty(sfn)) return; end;
zl = ldimp(zs,ps,pl);
pr = (1 + z_chr ./ zl) / 2;
show_thl(f,zl,pl,pr,2,cfn);
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

% get cavity pressures from CAL file
function [f,pc,rate]=load_cal(fn)
global tcav dcav
PROBE_CAL=0;
HEAR_CAL=0;
if (isoctave)
   load(fn);    	% fetch data
else
   load(fn,'-MAT');	% fetch data
end
if (PROBE_CAL > 0)
   ncav = PROBE_CAL;
elseif (HEAR_CAL > 0)
   ncav = HEAR_CAL;
else
   error(sprintf('Invalid CAL file %s',fn));
end
n = npts / 2;
f = (0:n)*(rate/npts);
pc = [   ffa(tok1)./ffa_stm(stm1)];
if (ncav > 1) pc = [pc ffa(tok2)./ffa_stm(stm2)]; end
if (ncav > 2) pc = [pc ffa(tok3)./ffa_stm(stm3)]; end
if (ncav > 3) pc = [pc ffa(tok4)./ffa_stm(stm4)]; end
if (ncav > 4) pc = [pc ffa(tok5)./ffa_stm(stm5)]; end
% car = [reps attn ppc tcav dcav vpc adsn mpsn]
tcav = car1(4);
dcav = car1(5);
scale = car1(3) / car1(6);
pc = pc * scale;
fprintf('%s\n',fn);
return

% get contents of THS file
function [f,zs,ps] = load_ths(fn)
if (isoctave)
   load(fn);
else
   load(fn,'-MAT');
end
n=length(ps);
f=(0:(n-1))*df;
fprintf('%s\n',fn);
return

function show_thl(f,zl,pl,pr,fig,lab)
global f_plt f_pri
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
text(2,0.35,lab)
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
a=mean(mean([PL(ii) PF(ii)]));
axis([f_plt a-40 a+40]);
ylabel('magnitude (dB)')
title('load pressure');
subplot(2,2,4);
plot(fk,ph1,fk,ph2)
axis([f_plt -1.04 1.04]);
ylabel('phase (cyc)')
xlabel('frequency (kHz)')
text(2,0.7,s)
return

function slp=slope(x,y)
n=length(x);             % size of both x & y
sx=sum(x);               % sum of x
sy=sum(y);               % sum of y
sxx=dot(x,x);            % sum of x^2
sxy=dot(x,y);            % sum of x*y
slp=(n*sxy-sx*sy)/(n*sxx-sx*sx); % slope
return

function zl = ldimp(zs,ps,pl)
[nf,nc]=size(pl);
for k=1:nc
   zl(:,k)=zs.*pl(:,k)./(ps-pl(:,k));
end
zl(abs(zl)<eps)=eps;
return

function z0 = z_chr
global tcav dcav
d = tcav - 26.85;
r = dcav / 2;
c = 3.4723e4 * (1 + 0.00166 * d);
rho = 1.1769e-3 * (1 - 0.00335 * d);
z0 = (rho * c) / (pi * r^2);
return

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% fast Fourier analyze stimulus
function S=ffa_stm(s)
if (ischar(s))
   if (strcmp(s,'swp.ils'))
      load('swp.mat');
      s = data;
   else
      error(sprintf('Can''t open stimulus file %s.\n',s));
   end
end
S = ffa(s(:));
return

% fast Fourier analyze real signal
function H=ffa(h)
H=fft(real(h));
n=length(H);
m=1+n/2;            % assume n is even
H(1,:)=real(H(1,:));
H(m,:)=real(H(m,:));
H((m+1):n,:)=[];    % remove upper frequencies
return

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function o = isoctave
o=1;eval('OCTAVE_VERSION;','o=0;');

