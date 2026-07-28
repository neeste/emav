% cal.m - Show contents of EMAV CAL file.
function cal
select = 1;
fn = get_fn('*.CAL',select);
if (isempty(fn)) return; end;
if (select)
   plot_cal(fn,1)   
else
   for k=1:length(fn)
      plot_cal(fn(k).name,k)
   end
end
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

function plot_cal(fn,fig)
fprintf('%s\n',fn);
[pct1,f,pcf1,rate]=load_cal(fn,1);
[pct2,f,pcf2,rate]=load_cal(fn,2);
figure(fig);clf
%
t=((1:length(pct1))-1)*1000/rate;
flim=[0 16];
j=find(f>(flim(1)*1000+100)&f<(flim(2)*1000-100));
fj=f(j)/1000;
nj=length(j);
%
subplot(2,1,1);
pmx=max([max(abs(pct1)) max(abs(pct2))])*1.05;
plot(t,pct1,'b',t,pct2,'r')
axis([0 max(t) -pmx pmx]);
xlabel('time (ms)')
ylabel('(Pa)')
title(sprintf('%s - %.0f kHz',fn,rate/1000))
subplot(2,1,2);
P1=20*log10(abs(pcf1(j))+eps);
P2=20*log10(abs(pcf2(j))+eps);
plot(fj,P1,'b',fj,P2,'r')
dbmx=max([P1;P2])+5;
axis([flim dbmx-40 dbmx])
xlabel('frequency (kHz)')
ylabel('magnitude (dB)')
return

% get cavity pressures from CAL file
function [pct,f,pcf,rate]=load_cal(fn,ch)
if (isoctave)
   load(fn);    	% fetch data
else
   load(fn,'-MAT');	% fetch data
end
n = npts / 2;
f = (0:n)*(rate/npts);
if (ch == 1)
   pct = cal01(:) .* scale;
else
   pct = cal02(:) .* scale;
end
dasn = 6735; % Indigo two-channel average
pcf = dasn * ffa(pct) ./ ffa(stim(:));
return

% get load pressure from CAL file
function [pct,f,pcf,rate]=old_load_cal(fn,ch)
if (isoctave)
   load(fn);                     % load OCTAVE data
else
   load(fn,'-MAT');              % load MATLAB data
end
n = npts / 2;
f = (0:n)*(rate/npts);
if (ch == 1)                    % left channel
   pct = cal01(:) * scale;
else                            % right channel
   pct = cal02(:) * scale;
end
st = ffa(stim(:));
st=1;
pcf = [ffa(pct)./st];
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

function db=decibel(pr)
db=20*log10(max(pr,eps));
return

function o = isoctave
o=1;eval('OCTAVE_VERSION;','o=0;');
return

