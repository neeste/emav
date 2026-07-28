% thl.m - Show contents of EMAV THL files
%
function thl(fn,fig)
%
if (nargin < 1)	% show all files ?
   fn=dir('*.thl');
   for i=1:length(fn)
     show_thl(fn(i).name,i);
   end
   return
end
if (nargin < 2)
   fig = 1;
end
show_thl(fn,fig)
return
%
function show_thl(fn,fig)
if (isoctave)
   load(fn);   		% fetch data
else
   load(fn,'-MAT');	% fetch data
end
figure(fig);clf
n=length(pl1);
f=(0:(n-1))*df/1000;
j=find(f>0.5&f<9.5);
fj=f(j);
nj=length(j);
%
ZL1=20*log10(abs(zl1(j))+eps);
ZL2=20*log10(abs(zl2(j))+eps);
Z0=20*log10(abs(z0(j))+eps);
ph1=unwrap(angle(zl1(j)))/(2*pi);
ph2=unwrap(angle(zl2(j)))/(2*pi);
ph3=unwrap(angle(z0(j)))/(2*pi);
subplot(2,2,1);
plot(fj,ZL1,'b',fj,ZL2,'r',fj,Z0,'c')
axis([0 10 6 64]);
ylabel('magnitude (dB)')
title('load impedance');
subplot(2,2,3);
plot(fj,ph1,'b',fj,ph2,'r',fj,ph3,'c')
axis([0 10 -0.52 0.52]);
ylabel('phase (cyc)')
xlabel('frequency (kHz)')
title(fn)
%
pf1=pl1 .* pr1;
pf2=pl2 .* pr2;
PL1=20*log10(abs(pl1(j))+eps);
PL2=20*log10(abs(pl2(j))+eps);
PF1=20*log10(abs(pf1(j))+eps);
PF2=20*log10(abs(pf2(j))+eps);
ph1=unwrap(angle(pl1(j)))/(2*pi);
ph2=unwrap(angle(pl2(j)))/(2*pi);
ph3=unwrap(angle(pf1(j)))/(2*pi);
ph4=unwrap(angle(pf2(j)))/(2*pi);
gd=-(dot(ph1,fj)-sum(ph1)*mean(fj))/(dot(fj,fj)-sum(fj)*mean(fj));
ph0=(sum(ph1)+gd*sum(fj))/nj;
pt=ph0-gd*fj';
ph1=ph1-pt;
ph2=ph2-pt;
ph3=ph3-pt;
ph4=ph4-pt;
ph1=ph1-round(mean(ph1));
ph2=ph2-round(mean(ph2));
ph3=ph3-round(mean(ph3));
ph4=ph4-round(mean(ph4));
s=sprintf('delay=%.2f (ms)',gd);
subplot(2,2,2);
plot(fj,PL1,'b',fj,PL2,'r',fj,PF1,'g',fj,PF2,'c')
a=mean(mean([PL1 PL2 PF1 PF2]));
axis([0 10 a-30 a+30]);
ylabel('magnitude (dB)')
title('load pressure');
subplot(2,2,4);
plot(fj,ph1,'b',fj,ph2,'r',fj,ph3,'g',fj,ph4,'c')
axis([0 10 -1.04 1.04]);
ylabel('phase (cyc)')
xlabel('frequency (kHz)')
text(2,0.7,s)
%
Z0=20*log10(abs(mean(z0(j)))+eps);
ZL1=20*log10(abs(mean(zl1(j)))+eps);
ZL2=20*log10(abs(mean(zl2(j)))+eps);
fprintf('%s: z0=%4.1f zl1=%4.1f zl2=%4.1f (average impedance)\n', fn, Z0, ZL1, ZL2);
%
return

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function o = isoctave
o=1;eval('OCTAVE_VERSION;','o=0;');
