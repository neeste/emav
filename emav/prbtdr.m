% prbtdr - show time-domain reflectance for PRB files created by EMAV
function prbtdr
global f_plt f_pri f_err xtk dcav tcav fj pj fdw
f_plt = [0 12];         % plot frequency range (kHz)
f_pri = [2 10];         % primary frequency range (kHz)
f_err = [2  8];         % error frequency range (kHz)
xtk = 1;                % estimate crosstalk ?
fdw=1;                  % frequency-domain window ?
%
dcav = 0.8;
tcav = 25;
%
fnlst = get_fn('*.PRB',0);
if (isempty(fnlst)) return; end;
fig=0;
for j=1:length(fnlst)
   fn=fnlst(j).name;
   fprintf('%s: ',fn);
   figure(j);clf
   for ch=1:2
      [f,pc,rate]=load_prb(fn,ch);
      db=20*log10(max(eps,abs(pc)));
      dba(ch)=10*log10(mean(mean(10.^(db/10))));
      % initialize
      lc=cavlen(f,pc);                		% estimate cavity lengths
      zc=cavimp(f,lc);                		% calculate cavity impedances
      %
      df=f(2)-f(1);
      jef1=1+round(f_err(1)*1000/df);
      jef2=1+round(f_err(2)*1000/df);
      ej=jef1:jef2;
      pj=pc(ej,:);
      fj=f(ej);
      %
      [zs,ps]=thvsrc(zc,pc);          		% estimate zs & ps
      % iterate
      tic;lc=fminsearch('thverr',lc,opt_set);et=toc;
      err(ch)=thverr(lc);                 % final error
      zc=cavimp(f,lc);                		% calculate cavity impedances
      [zs,ps]=thvsrc(zc,pc);          		% estimate zs & ps
      zl=ldimp(zs,ps,pc);
      for k=1:length(lc)
         c=plt_color(k);
         show_tdr(rate,zl(:,k),fig,fn,c)	% plot zl1 & zl2
      end
   end
   hold off
   fprintf('dB_avg=%5.1f,%5.1f; ',dba(1),dba(2));
   fprintf('err=%6.3f,%.3f\n',err(1),err(2));
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

% plot time-domain reflectance
function show_tdr(rate,zl,n,fn,c)
mf=4;
%
zo=surge(zl);
R=(zl-zo)./(zl+zo);
r=ffs(fd_window(R,mf));
r=fftshift(r);
n=length(r)/2;
t=((-n):(n-1))*1000/rate/mf;
lim=[-0.2 1 -0.05 0.45];
plot(t,r,c)
axis(lim)
title(fn);
xlabel('time (ms)')
ylabel('reflectance')
hold on
%
drawnow
return

function lc=cavlen(f,pc)
[nr,nc]=size(pc);
lc=zeros(1,nc);
rate=2*max(f);
for k=1:nc
   lc(k) = cavity_length(pc(:,k),rate);
end
return

function [lk,td]=cavity_length(pc,rate)
global tcav dcav
lmn=1;		% minimum cavity length (cm)
lmx=12;		% maximum cavity length (cm)
dtc = tcav - 26.85;
c = 3.4723e4 * (1 + 0.00166 * dtc);
%
imn=round(lmn*rate*2/c)+1;
imx=round(lmx*rate*2/c)+1;
p=ffs(abs(pc).^2);
[x,m]=max(p(imn:imx));m=m+imn-1;
d =(p(m-1)-p(m+1))/(p(m-1)-2*p(m)+p(m+1))/2;
td =(m+d-1)/rate;
lk=td*c/2;
return

function zl = ldimp(zs,ps,pl)
[nf,nc]=size(pl);
for k=1:nc
   zl(:,k)=zs.*pl(:,k)./(ps-pl(:,k));
end
return

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% fast Fourier analyze real signal
function H=ffa(h)
H=fft(real(h));
n=length(H);
m=1+n/2;            % assume n is even
H(1,:)=real(H(1,:));
H(m,:)=real(H(m,:));
H((m+1):n,:)=[];    % remove upper frequencies
return

function H=fd_window(H,mf)
global fdw
if (fdw)
   n=length(H)-1;
   p=pi*(0:n)'/n;
   a=0.16; % Blackman window
   w=(1-a+cos(p)+a*cos(2*p))/2;
   H=H.*w;
   H=[H;zeros((mf-1)*n,1)]*mf;
end
return

function zo=surge(zl)
zo=mean(real(zl));
wo=mean(fd_window(ones(size(zl)),1));
for k=1:8
    wr=mean(real(fd_window((zl-zo)./(zl+zo),1)));
    zo=zo*(1+wr/wo);
 end
return

% fast Fourier synthesize real signal
function h=ffs(H)
m=length(H);
n=2*(m-1);
H(1,:)=real(H(1,:));
H(m,:)=real(H(m,:));
H((m+1):n,:)=conj(H((m-1):-1:2,:));
h=real(ifft(H));
return

function c=plt_color(seq)
colors=['b' 'g' 'r' 'c' 'm' 'k'];
c=colors(seq);
return

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function op = opt_set
if (isoctave)
   op=[0 0 0 0 0 0 0 0 0 9000];
else
   op=optimset('Display','off','MaxIter',9000);
end
return

function flush
if (isoctave)
   fflush(stdout);
end
return

function o = isoctave
o=1;eval('OCTAVE_VERSION;','o=0;');

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [zc,z0] = cavimp(f,L)
[z0,gam] = cav0(f);
R = exp(-2 * gam * L);
zc = z0 .* (1 + R) ./ (1 - R);
return

% Keefe (1984) tube equations
function [z0,wn] = cav0(f)
global tcav dcav
d = tcav - 26.85;
r = dcav / 2;
c = 3.4723e4 * (1 + 0.00166 * d);
rho = 1.1769e-3 * (1 - 0.00335 * d);
eta = 1.846e-4 * (1 + 0.0025 * d);
w = 2 * pi * f(:);
w(w<eps) = eps;   % avoid w=0
Ro = (rho * c) / (pi * r^2);
Rv = r * sqrt(rho * w / eta);
x = (1.045 + (1.080 + 0.750 ./ Rv) ./ Rv) ./ Rv;
y = 1 + 1.045 ./ Rv;
wn = (w / c) .* complex(x,y);
z0 = Ro;            % char. imped. is constant & real
return

% estimate Thevinen-equivalent source pressure & impedance
function [zs,ps] = thvsrc(zc,pc)
[nf,nc]=size(pc);
zs=zeros(nf,1);
ps=zeros(nf,1);
for  k=1:nf
   z = zc(k,:).';
   p = pc(k,:).';
   A = [z   -p];
   B =  z .* p ;
   x = A \ B;
   ps(k) = x(1);
   zs(k) = x(2);
end
return

