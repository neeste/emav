% calmat.m - view contents of a MATLAB data file created by CALMAT
%            from an EMAV .CAL file.
%
function calmat
load 93C25D00.CAL -MAT		% specify the filename
a=cal1*(ppc/reps);
b=cal2*(ppc/reps);
figure(1);clf;
subplot(2,1,1);
wave(a,b,rate);
subplot(2,1,2);
spec(a,b,rate);
return

function wave(a,b,rate)
t=(0:(length(a)-1))*1000/rate;
plot(t,a,'r',t,b,'b');
xlabel('time (msec)');
axis([min(t) max(t) min(a) max(a)]);
return

function spec(a,b,rate)
n=length(a);
f=linspace(0,rate/1000,n);
A=fft(a);
B=fft(b);
r=n*(20e-6*sqrt(2));
eps=1e-9;
C=20*log10(max(abs(A)/r,eps));
D=20*log10(max(abs(B)/r,eps));
t=1+round(max(C));
b=t-80;
plot(f,C,'r',f,D,'b');
axis([0 10 b t]);
xlabel('frequency (kHz)');
return

