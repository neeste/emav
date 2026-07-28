% binmat.m - view contents of a MATLAB data file created by BINMAT
%            from an EMAV .BIN file.
%
function binmat
load 03C05D01		% specify the filename
n=length(f1);
for i=1:n
   fprintf('==== condition %d/%d ====\n', i, n);
   an=sprintf('A%03d',i);
   bn=sprintf('B%03d',i);
	a=eval(an)*int2volt*volt2pas/nswp(i);
	b=eval(bn)*int2volt*volt2pas/nswp(i);
   %wave(a,b,rate,nswp(i));
   spec(a,b,rate,nswp(i),f1(i),f2(i),L1(i),L2(i));
   pause(8);
end

function wave(a,b,rate,nswp)
t=(0:(length(a)-1))/rate;
plot(t,a,t,b);

function spec(a,b,rate,nswp,f1,f2,L1,L2)
n=length(a);
f=linspace(0,rate/1000,n);
A=fft(a);
B=fft(b);
r=n*(20e-6*sqrt(2));
eps=1e-9;
C=20*log10(max(abs(A+B)/r,eps));
D=20*log10(max(abs(A-B)/r,eps));
t=1+round(max(C));
b=t-80;
plot(f,C,f,D);
axis([0 5 b t]);
i1=1+round(n*f1/rate);
i2=1+round(n*f2/rate);
id=2*i1-i2;
fprintf('F2/F1 = %.0f/%.0f = %.3f\n', f2, f1, f2 / f1);
fprintf('L1-L2 = %.0f-%.0f = %.0f\n', L1, L2, L1 - L2);
fprintf('Fdp = %.0f Hz\n', 2 * f1 - f2);
fprintf('A+B: L1=%.1f L2=%.1f Ld=%.1f dB SPL\n', C(i1), C(i2), C(id));
%fprintf('A-B: L1=%.1f L2=%.1f Ld=%.1f dB SPL\n', D(i1), D(i2), D(id));

