% ths.m - Show contents of EMAV THS file.
%
function ths
global f_plt f_pri
f_plt = [0 12];         % plot frequency range (kHz)
f_pri = [2 10];         % primary frequency range (kHz)
fn = get_fn('*.THS',1);
if (isempty(fn)) 
   fprintf('No THS files.\n');
   return; 
end
plot_ths(fn,1)
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
      n  = input(sprintf('\nWhich file (1-%d)? [1] \n',nf));
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

function plot_ths(fn,fig)
global f_plt f_pri
fprintf('%s\n',fn);
if (isoctave)
   load(fn);   		% fetch data
else
   load(fn,'-MAT');	% fetch data
end
figure(fig);clf
n=length(ps1);
fk=((0:(n-1))*df/1000)';
ii = find(fk>=f_pri(1) & fk<=f_pri(2));
%
ZS1=20*log10(max(abs(zs1),eps));
ZS2=20*log10(max(abs(zs2),eps));
ph1=angle(zs1)/(2*pi);
ph2=angle(zs2)/(2*pi);
subplot(2,2,1);
plot(fk,ZS1,'b',fk,ZS2,'r')
axis([f_plt 0 80]);
ylabel('magnitude (dB \Omega)')
title('source impedance');
subplot(2,2,3);
plot(fk,ph1,'b',fk,ph2,'r')
axis([f_plt -0.55 0.55]);
ylabel('phase (cyc)')
xlabel('frequency (kHz)')
text(2,0.35,fn)
%
PS1=20*log10(max(abs(ps1),eps));
PS2=20*log10(max(abs(ps2),eps));
ph1=unwrap(angle(ps1))/(2*pi);
ph2=unwrap(angle(ps2))/(2*pi);
[nf,nc]=size(ph1);
gd=-slope(fk(ii),ph1(ii,1));
ph1=ph1+fk*ones(1,nc)*gd;
ph2=ph2+fk*ones(1,nc)*gd;
ph1=ph1-ones(nf,1)*round(mean(ph1(ii,:)));
ph2=ph2-ones(nf,1)*round(mean(ph2(ii,:)));
subplot(2,2,2);
plot(fk,PS1,'b',fk,PS2,'r')
a=mean(mean([PS1 PS2]));
axis([f_plt a-40 a+40]);
ylabel('magnitude (dB Pa/V)')
title('source pressure');
subplot(2,2,4);
plot(fk,ph1,'b',fk,ph2,'r')
axis([f_plt -1 1]);
ylabel('phase (cyc)')
xlabel('frequency (kHz)')
text(5,0.7,sprintf('delay=%.2f ms',gd));
fprintf('avg. pressure = %.1f\n', a)
return

function slp=slope(x,y)
n=length(x);             % size of both x & y
sx=sum(x);               % sum of x
sy=sum(y);               % sum of y
sxx=dot(x,x);            % sum of x^2
sxy=dot(x,y);            % sum of x*y
slp=(n*sxy-sx*sy)/(n*sxx-sx*sx); % slope
return

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function o = isoctave
o=1;eval('OCTAVE_VERSION;','o=0;');
