% tokmat.m - view contents of a MATLAB data file created by TOKMAT
%            from an EMAV .TOK file.
%
function tokmat
load 92L21T00		% specify the filename
s=tok1*(ppc/reps);
a=tok2*(ppc/reps);
b=tok3*(ppc/reps);
t=(0:(length(a)-1))*1000/rate;
figure(1);clf;
subplot(2,1,1);
plot(t,s,'g');
xlabel('time (msec)');
axis([min(t) max(t) min(a) max(a)]);
subplot(2,1,2);
plot(t,a,'r',t,b,'b');
xlabel('time (msec)');
axis([min(t) max(t) min(a) max(a)]);
return

