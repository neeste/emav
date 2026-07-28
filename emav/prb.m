% prb - show contents of PRB file
function prb
global f_plt f_pri
f_plt = [0 20];         % plot frequency range (kHz)
f_pri = [2 16];         % primary frequency range (kHz)
%
fn = get_fn('*.PRB',1);
if (isempty(fn)) return; end;
[f,pc1,rate]=load_prb(fn,1);
[f,pc2,rate]=load_prb(fn,2);
show_pc(f,pc1,pc2,1,fn);
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

% get cavity pressures from PRB file
function [f,pc,rate]=load_prb(fn,ch)
da1sn=6922; % default for CardDeluxe
da2sn=da1sn;
if (isoctave)
   load(fn);    	% fetch data
else
   load(fn,'-MAT');	% fetch data
end
n = npts / 2;
f = (0:n)*(rate/npts);
st = ffa(stim(:));
st(abs(st)<eps)=eps;
if (ch == 1)      % left channel
   pc = [   ffa(cal01(:))./st];
   if (ncav > 1) pc = [pc ffa(cal03(:))./st]; end
   if (ncav > 2) pc = [pc ffa(cal05(:))./st]; end
   if (ncav > 3) pc = [pc ffa(cal07(:))./st]; end
   if (ncav > 4) pc = [pc ffa(cal09(:))./st]; end
   if (ncav > 5) pc = [pc ffa(cal11(:))./st]; end
   dasn = da1sn;
else              % right channel
   pc = [   ffa(cal02(:))./st];
   if (ncav > 1) pc = [pc ffa(cal04(:))./st]; end
   if (ncav > 2) pc = [pc ffa(cal06(:))./st]; end
   if (ncav > 3) pc = [pc ffa(cal08(:))./st]; end
   if (ncav > 4) pc = [pc ffa(cal10(:))./st]; end
   if (ncav > 5) pc = [pc ffa(cal12(:))./st]; end
   dasn = da2sn;
end
pc = pc * (scale * dasn * 10^(attn/20) * 2);
return

function slp=slope(x,y)
n=length(x);             % size of both x & y
sx=sum(x);               % sum of x
sy=sum(y);               % sum of y
sxx=dot(x,x);            % sum of x^2
sxy=dot(x,y);            % sum of x*y
slp=(n*sxy-sx*sy)/(n*sxx-sx*sx); % slope
return

% plot pressure
function show_pc(f,pc1,pc2,fig,fn)
global f_plt f_pri
figure(fig);clf
fk = f(:)/1000;
ii = find(f>100 & f<9000);
ii=find(fk>=f_pri(1) & fk<=f_pri(2));
%
[np,nc] = size(pc1);
ncav = nc;
ncol = 2;
for k=1:ncol
   lab=sprintf('%d cavities, source %d',ncav,k);
   if (k == 1)
      p = pc1;
   else
      p = pc2;
   end
   m = k + (0:(ncav-1))*ncol;
   subplot(2,ncol,k)
   db=20*log10(max(eps,abs(p)));
   plot(fk,db)
   dbavg=10*log10(mean(mean(10.^(db/10))));
   fprintf('%s-%d: dB_avg=%.1f\n',fn,k,dbavg);
   ym = max(max(db(ii)));
   axis([f_plt ym-70 ym+10])
   title(lab)
   ylabel('pressure (dB Pa/V)')
   subplot(2,ncol,k+ncol)
   ph=unwrap(angle(p))/(2*pi);
   [nf,nc]=size(ph);
   gd=-slope(fk(ii),ph(ii));
   ph=ph+fk*ones(1,nc)*gd;
   ph=ph-ones(nf,1)*round(mean(ph(ii,:)));
   plot(fk,ph)
   axis([f_plt -1 1])
   xlabel('frequency (kHz')
   ylabel('phase (cyc)')
   if (k==1) title(fn); end
   text(mean(f_plt),0.7,sprintf('delay=%.2f ms',gd));
end
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

function o = isoctave
o=1;eval('OCTAVE_VERSION;','o=0;');
return

