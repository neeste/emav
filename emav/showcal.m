function showcal
% function showcan(fn)
% fn - EMAV calibration file in MAT format
load '07C16D00.CAL' -MAT
clf
t=(0:(npts-1))/rate;
m=max(max(abs(cal01)),max(abs(cal02)))*1.05;
subplot(2,1,1)
plot(t,cal01,'r',t,cal02,'b');
axis([0 max(t) -m m]);
xlabel('time');
%
CAL1=fft(cal01);
CAL2=fft(cal02);
i=1:(1+npts/2);
f=(i-1)*(rate/npts);
S1=dB(CAL1(i));
S2=dB(CAL2(i));
m=max(max(S1),max(S2)) + 5;
subplot(2,1,2)
plot(f,S1,'r',f,S2,'b');
axis([0 max(f) -m m]);
xlabel('frequency');
return

function y=dB(x)
y=20*log10(max(abs(x),eps));
return

