% calplt.m - plot contents of a PUTT CAL file.
%
function calplt(ccn,pcn)
global tcav dcav
%
if (nargin<1) ccn = get_cfn(1); end
if (nargin<2) pcn = get_pfn(1); end
fprintf('%s\n',ccn);
fprintf('%s\n',pcn);
%
pfn = strcat(pcn,'.CAL'); % probe file
sfn = strcat(pcn,'.THS'); % Thevenin source file
cfn = strcat(ccn,'.CAL'); % cavity file
lfn = strcat(ccn,'.THL'); % Thevenin load file
%
% probe calibration
%
if (isoctave)
   load(pfn)
else
   load(pfn,'-mat')
end
% car = [reps attn ppc tcav dcav vpc adsn mpsn]
n=length(tok1);
fig=1;figure(fig);clf
s=[stm1 stm2 stm3 stm4 stm5]*car1(6);
r=[tok1 tok2 tok3 tok4 tok5]*car1(3);
subplot(2,1,1);
wave(r,rate);
subplot(2,1,2);
spec(r,rate);
rc=r;
%
% cavity calibration
%
if (isoctave)
   load(cfn)
else
   load(cfn,'-mat')
end
fig=fig+1;figure(fig);clf
s=stm1*car1(6);
r=tok1*car1(3);
subplot(2,1,1);
wave(r,rate);
subplot(2,1,2);
spec(r,rate);
%
% source, load, & crosstalk pressure
%
if (isoctave)
   load(sfn)
else
   load(sfn,'-mat')
end
if (isoctave)
   load(lfn)
else
   load(lfn,'-mat')
end
fig=fig+1;figure(fig);clf
pc=(fft(r)./fft(s));
PC=20*log10(abs(pc)+eps);
PL=20*log10(abs(pl)+eps);
PS=20*log10(abs(ps)+eps);
PX=20*log10(abs(px)+eps);
k=2:min(length(PL),length(PS));
f=(0:(n-1))*(rate/n)/1000;
ff=f(k);
plot(ff,PC(k),ff,PL(k),ff,PS(k),ff,PX(k));
t=1+round(max(max(PS(k))));
axis([0 10 t-80 t]);
xlabel('frequency (kHz)');
ylabel('(dB)');
title('source, load, & crosstalk pressure');
d=mean(mean(PC(k)-PL(k)));
fprintf('consistency check: mean(PC-PL)=%.3f dB\n',d);
%
% cavity & cross-talk pressure
%
fig=fig+1;figure(fig);clf
hold on
spec(rc,rate);
plot(ff,PX(k),'--');
axis([0 10 -50 50]);
title('cavity & cross-talk pressure');
xlabel('frequency (kHz)');
ylabel('pressure (dB)')
hold off
%
% source pressure
%
fig=fig+1;figure(fig);clf
p0=ps(k);
P0=20*log10(abs(p0)+eps);
s=stm1;
sw=fft(s);
s0=sw(k);
phs=unwrap(angle(p0))/(2*pi);
phs=phs-trend(phs);
subplot(2,1,1);
plot(ff,P0);
t=1+round(max(P0));
axis([0 10 t-80 t]);
title('source pressure');
ylabel('(dB)');
subplot(2,1,2);
plot(ff,phs);
xlim([0 10]);
xlabel('frequency (kHz)');
ylabel('(cyc)');
%
% source impedance
%
fig=fig+1;figure(fig);clf
z0=zs(k);
Z0=20*log10(abs(z0)+eps);
phs=angle(z0)/(2*pi);
subplot(2,1,1);
plot(ff,Z0);
t=1+round(max(max(Z0)));
axis([0 10 t-80 t]);
ylabel('(dB)');
title('source impedance');
subplot(2,1,2);
plot(ff,phs);
xlim([0 10]);
xlabel('frequency (kHz)');
ylabel('(cyc)');
%
% source pressure and impedance
%
fig=fig+1;figure(fig);clf
subplot(2,1,1);
plot(ff,P0);
t=1+round(max(max(P0)));
axis([0 10 t-50 t]);
ylabel('pressure (dB)')
title('source');
subplot(2,1,2);
plot(ff,Z0);
t=1+round(max(max(Z0)));
axis([0 10 t-50 t]);
ylabel('impedance (dB)')
xlabel('frequency (kHz)')
%
% load impedance
%
fig=fig+1;figure(fig)
clf
p1=pl(k)-px(k);
z1=zl(k);
z2=zs(k).*p1./(ps(k)-p1);
Z1=20*log10(abs(z1)+eps);
Z2=20*log10(abs(z2)+eps);
ph1=angle(z1)/(2*pi);
ph2=angle(z2)/(2*pi);
subplot(2,1,1);
plot(ff,Z1,ff,Z2);
t=1+round(max(max(Z1)));
axis([0 10 t-80 t]);
ylabel('(dB)');
title('load impedance');
subplot(2,1,2);
plot(ff,ph1,ff,ph2);
xlim([0 10]);
xlabel('frequency (kHz)');
ylabel('(cyc)');
%
% load conductance
%
fig=fig+1;figure(fig)
clf
g1=max(real(1./z1),eps);
g2=max(real(1./z2),eps);
G1=10*log10(abs(g1)+eps);
G2=10*log10(abs(g2)+eps);
plot(ff,G1,ff,G2);
t=1+round(max(G1));
axis([0 10 t-80 t]);
xlabel('frequency (kHz)');
ylabel('(dB)');
title('load conductance');
return

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% get PROBE_CAL filename
function fn=get_pfn(select)
fn = dir('*.CAL');
fn = select_probe(fn);
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
   fn = fn(1:8);
end
return

% select PROBE_CAL files
function fn=select_probe(fn)
ii=ones(size(fn));
for k=1:length(fn);
   clear PROBE_CAL
   load(fn(k).name,'-mat');
	eval('PROBE_CAL;','ii(k)=0;');
end
fn=fn(logical(ii));
return

% get PROBE_CAL filename
function fn=get_cfn(select)
fn = dir('*.CAL');
fn = select_hear(fn);
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
   fn = fn(1:8);
end
return

% select PROBE_CAL files
function fn=select_hear(fn)
ii=ones(size(fn));
for k=1:length(fn);
   clear PROBE_CAL
   load(fn(k).name,'-mat');
	eval('HEAR_CAL;','ii(k)=0;');
end
fn=fn(logical(ii));
return

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function wave(a,rate)
t=(0:(length(a)-1))*1000/rate;
plot(t,a);
xlabel('time (msec)');
axis([min(t) max(t) min(min(a)) max(max(a))]);
return
%
function spec(a,rate)
n=length(a);
f=linspace(0,rate/1000,n);
ref=20e-6*sqrt(2)*n;
eps=1e-9;
A=fft(a);
G=20*log10(abs(A)/ref+eps);
plot(f,G);
t=1+round(max(max(G)));
axis([0 10 t-80 t]);
xlabel('frequency (kHz)');
ylabel('(dB)');
return
%
function s=gen_sweep(n,rate)
df = (rate / 2) / (n - 1);
am = 32767; % MAXPOSINT
ph = 0;
s=zeros(n,1);
for k=1:n
	ph = ph + 2 * pi * k * df / rate;
	s(k) = round(am * sin(ph));
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function o = isoctave
o=1;eval('OCTAVE_VERSION;','o=0;');

