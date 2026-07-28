% prbsrc - estimate Thevenin-equivalent source pressure and impedance for PRB file created by EMAV
function prbsrc
global f_plt f_pri f_err xtk dcav tcav fj pj
f_plt = [0 12];         % plot frequency range (kHz)
f_pri = [2 10];         % primary frequency range (kHz)
f_err = [2  8];         % error frequency range (kHz)
xtk = 1;                % estimate crosstalk ?
%
dcav = 0.8;
tcav = 25;
%
fn = get_fn('*.PRB',1);
if (isempty(fn)) return; end;
fig=0;
for ch=1:2
   fprintf('source channel #%d\n',ch);
   [f,pc,rate]=load_prb(fn,ch);
   fnch=sprintf('%s-%d',fn,ch);
   % initialize
   lc=cavlen(f,pc);                		% estimate cavity lengths
   zc=cavimp(f,lc);                		% calculate cavity impedances
   fig=fig+1;
   show_pc(f,pc,fig,fnch)  % plot pc
   %
   df=f(2)-f(1);
   jef1=1+round(f_err(1)*1000/df);
   jef2=1+round(f_err(2)*1000/df);
   ej=jef1:jef2;
   pj=pc(ej,:);
   fj=f(ej);
   %
   [zs,ps]=thvsrc(zc,pc);          		% estimate zs & ps
   fig=fig+1;
   show_zp(f,zs,ps,fig,fnch,'source')  % plot zs & ps
   prn_len('initial',lc);
   % iterate
   tic;lc=fminsearch('thverr',lc,opt_set);et=toc;
   prn_len('  final',lc);
   zc=cavimp(f,lc);                		% calculate cavity impedances
   [zs,ps]=thvsrc(zc,pc);          		% estimate zs & ps
   [zs,ps] = smooth_src(zc,pc,zs,ps,rate);% smooth ???
   show_zp(f,zs,ps,fig,fnch,'source')  % plot zs & ps
   zl=ldimp(zs,ps,pc);
   fig=fig+1;
   show_zz(f,zl,zc,fig,fnch,'measured & ideal')	% plot zc & zl
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

% plot pressure
function show_pc(f,pc,fig,fn)
global f_plt f_pri
figure(fig);clf
fk = f(:)/1000;
ii = find(f>100 & f<9000);
ii=find(fk>=f_pri(1) & fk<=f_pri(2));
%
[np,nc] = size(pc);
ncav = nc;
lab=sprintf('%d cavities',ncav);
p = pc;
%m = 1 + (0:(ncav-1))*ncol;
subplot(2,1,1)
db=20*log10(max(eps,abs(p)));
plot(fk,db)
dbavg=10*log10(mean(mean(10.^(db/10))));
fprintf('dB_avg=%.1f\n',dbavg);
ym = max(max(db(ii)));
axis([f_plt ym-70 ym+10])
title(lab)
ylabel('pressure (dB Pa/V)')
subplot(2,1,2)
ph=unwrap(angle(p))/(2*pi);
[nf,nc]=size(ph);
gd=-slope(fk(ii),ph(ii));
ph=ph+fk*ones(1,nc)*gd;
ph=ph-ones(nf,1)*round(mean(ph(ii,:)));
plot(fk,ph)
axis([f_plt -1 1])
xlabel('frequency (kHz')
ylabel('phase (cyc)')
title(fn);
text(mean(f_plt),0.7,sprintf('delay=%.2f ms',gd));
return

% plot impedance & pressure
function show_zp(f,z,p,n,fn,lab)
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
xlabel('frequency (kHz)')
ylabel('phase (cyc)')
text(2,0.35,fn)
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
drawnow
return

% plot measured & ideal impedance
function show_zz(f,z1,z2,n,fn,lab)
global f_plt f_pri
figure(n);clf
fk = f(:)/1000;
ii = find(fk>=f_pri(1) & fk<=f_pri(2));
%
subplot(2,1,1)
db1=20*log10(max(eps,abs(z1)));
db2=20*log10(max(eps,abs(z2)));
plot(fk,db1)
hold on
plot(fk,db2,'--')
hold off
ym = max(max(db1(ii)));
axis([f_plt ym-70 ym+10])
title(sprintf('%s impedance',lab))
ylabel('impedance (dB)')
%
subplot(2,1,2)
ph1=angle(z1)/(2*pi);
ph2=angle(z2)/(2*pi);
plot(fk,ph1)
hold on
plot(fk,ph2,'--')
hold off
axis([f_plt -0.55 0.55])
xlabel('frequency (kHz')
ylabel('phase (cyc)')
text(1,0.35,fn)
%
drawnow
return

function lc=prn_len(lab,lc)
err=thverr(lc);                 % final error
fprintf('%s lengths:',lab);
fprintf(' %5.2f',lc);
fprintf(' cm;  err=%.3f\n',err);%
return

function slp=slope(x,y)
n=length(x);             % size of both x & y
sx=sum(x);               % sum of x
sy=sum(y);               % sum of y
sxx=dot(x,x);            % sum of x^2
sxy=dot(x,y);            % sum of x*y
slp=(n*sxy-sx*sy)/(n*sxx-sx*sx); % slope
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

% fast Fourier analyze stimulus
function S=ffa_s(s)
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

% fast Fourier synthesize real signal
function h=ffs(H)
m=length(H);
n=2*(m-1);
H(1,:)=real(H(1,:));
H(m,:)=real(H(m,:));
H((m+1):n,:)=conj(H((m-1):-1:2,:));
h=real(ifft(H));
return

% smooth source pressure & impedance
function [zs,ps] = smooth_src(zc,pc,zs,ps,rate)
t1=0.020;	% positive time (s)
t2=0.002;   % negative time (s)
zs=smooth_imp(zs,rate,t1,t2);
for  k=1:length(zs)
   zl = zc(k,:).';
   pl = pc(k,:).';
   ps(k) = zl \ ((zl + zs(k)) .* pl);
end
return

% smooth impedance
function zs = smooth_imp(zs,rate,t1,t2)
nf=length(zs);
nt=(nf-1)*2;
zt=ffs(zs);
na=1+round(t1*rate/2);
n1=1+round(t1*rate);
n2=nt-round(t2*rate);
ia=(na:n1)';
ib=(n2:nt)';
zt=zt-mean(zt(ia));
zt(ia)=zt(ia).*(1+cos(pi*(ia-na)/(n1-na)))/2;
zt(ib)=zt(ib).*(1-cos(pi*(ib-n2)/(nt-n2)))/2;
zt(n1:n2)=0;
zs=ffa(zt);
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

