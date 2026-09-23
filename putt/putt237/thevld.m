% thevld - calculate load impedance from CAL & THS files
function thevld
global f_plt f_pri
f_plt = [0 12];         % plot frequency range (kHz)
f_pri = [2 10];         % primary frequency range (kHz)
%
fn = get_fn('*.CAL',1);
if (isempty(fn)) return; end;
[f,pl,rate]=load_cal(fn);
fn = get_fn('*.THS',1);
[f,zs,ps] = load_ths(fn);
if (isempty(fn)) return; end;
zl = ldimp(zs,ps,pl);
show_zp(f,zl,pl,2,'load')     % plot zc & pc
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
scale = car1(3) / car1(6);		%  * 67.6 ???
pc = pc * scale;
%
fprintf('%s\n',fn);
return

% get contents of THS file
function [f,zs,ps] = load_ths(fn)
if (isoctave)
   load(fn);    	% fetch data
else
   load(fn,'-MAT');	% fetch data
end
n=length(ps);
f=(0:(n-1))*df;
fprintf('%s\n',fn);
return

% plot impedance & pressure
function show_zp(f,z,p,n,lab)
global f_plt f_pri
figure(n);clf
fk = f(:)/1000;
ii = find(fk>=f_pri(1) & fk<=f_pri(2));
%
subplot(2,2,1)
db=20*log10(max(eps,abs(z)));
plot(fk,db)
ym = max(max(db(ii)));
axis([f_plt ym-70 ym+10])
title(sprintf('%s impedance',lab))
ylabel('impedance (dB)')
%
subplot(2,2,2)
db=20*log10(max(eps,abs(p)));
plot(fk,db)
ym = max(max(db(ii)));
axis([f_plt ym-70 ym+10])
title(sprintf('%s pressure',lab))
ylabel('magnitude (dB)')
%
subplot(2,2,3)
ph=angle(z)/(2*pi);
plot(fk,ph)
axis([f_plt -0.55 0.55])
xlabel('frequency (kHz')
ylabel('phase (cyc)')
%
subplot(2,2,4)
ph=unwrap(angle(p))/(2*pi);
[nf,nc]=size(ph);
gd=-slope(fk(ii),ph(ii,1));
ph=ph+fk*ones(1,nc)*gd;
ph=ph-ones(nf,1)*round(mean(ph(ii,:)));
plot(fk,ph)
axis([f_plt -1 1])
xlabel('frequency (kHz')
ylabel('phase (cyc)')
text(5,0.7,sprintf('delay=%.2f ms',gd));
%
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
