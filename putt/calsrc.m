% calsrc - estimate Thevenin-equivalent source pressure and impedance for CAL file created by PUTT
function calsrc
global f_plt f_pri f_err xtk dcav tcav fj pj
f_plt = [0 12];         % plot frequency range (kHz)
f_pri = [2 10];         % primary frequency range (kHz)
f_err = [2  8];         % error frequency range (kHz)
xtk = 1;                % estimate crosstalk ?
%
dcav = 0.8;
tcav = 25;
%
fn = get_fn('*.CAL',1);
if (isempty(fn)) return; end;
[f,pc,rate]=load_cal(fn);
% initialize
lc=cavlen(f,pc);                % estimate cavity lengths
zc=cavimp(f,lc);                % calculate cavity impedances
show_zp(f,zc,pc,1,fn,'cavity')  % plot zc & pc
%
df=f(2)-f(1);
jef1=1+round(f_err(1)*1000/df);
jef2=1+round(f_err(2)*1000/df);
ej=jef1:jef2;
pj=pc(ej,:);
fj=f(ej);
%
[zs,ps]=thvsrc(zc,pc);          % estimate zs & ps
show_zp(f,zs,ps,2,fn,'source')  % plot zs & ps
prn_len('initial',lc);
% iterate
tic;lc=fminsearch('thverr',lc,opt_set);et=toc;
prn_len('  final',lc);
zc=cavimp(f,lc);                % calculate cavity impedances
[zs,ps]=thvsrc(zc,pc);          % estimate zs & ps
show_zp(f,zs,ps,2,fn,'source')  % plot zs & ps
zl=ldimp(zs,ps,pc);
show_zz(f,zl,zc,3,'measured & ideal')	% plot zc & zl
return

% get filename
function fn=get_fn(fn,select)
fn = dir(fn);
fn = select_probe(fn);
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

% select PROBE_CAL files
function fn=select_probe(fn)
ii=ones(size(fn));
for k=1:length(fn);
   clear PROBE_CAL
   load(fn(k).name,'-mat');
	eval('PROBE_CAL;','ii(k)=0;');
end
fn=fn(logical(ii));
return

% get cavity pressures from CAL file
function [f,pc,rate]=load_cal(fn)
global tcav dcav
PROBE_CAL=0;
if (isoctave)
   load(fn);    	% fetch data
else
   load(fn,'-MAT');	% fetch data
end
if (PROBE_CAL > 0)
   ncav = PROBE_CAL;
else
   error(sprintf('Invalid PROBE_CAL file %s',fn));
end
n = npts / 2;
f = (0:n)*(rate/npts);
pc = [   ffa(tok1(:))./ffa_s(stm1)];
if (ncav > 1) pc = [pc ffa(tok2(:))./ffa_s(stm2)]; end
if (ncav > 2) pc = [pc ffa(tok3(:))./ffa_s(stm3)]; end
if (ncav > 3) pc = [pc ffa(tok4(:))./ffa_s(stm4)]; end
if (ncav > 4) pc = [pc ffa(tok5(:))./ffa_s(stm5)]; end
% car = [reps attn ppc tcav dcav vpc adsn mpsn]
tcav = car1(4);
dcav = car1(5);
scale = car1(3) / car1(6);
pc = pc * scale;
fprintf('%s\n',fn);
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
xlabel('frequency (kHz')
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
return

% plot measured & ideal impedance
function show_zz(f,z1,z2,n,lab)
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
%
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

% estimate Thevinen-equivalent source pressure & phase
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
