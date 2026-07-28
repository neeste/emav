% refl_td.m - Show reflectance from PUTT THL files
%
function refl_td
global fdw T xmax fmax
fdw=1;      % frequency-domain window ?
T=20;			% air temperature (deg C)
xmax=12;    % maximum tube length (cm)
fmax=20;    % maximum frequency (kHz)
%
fnlst=get_fn('*.THL');
if (isempty(fnlst)) 
   fprintf('No THL files.\n');
   return; 
end
plot_refl(fnlst); % plots dB & phase of pr
return

function filelist=get_fn(fp)
fn = dir(fp);
nfn=length(fn);
if (nfn < 1)
   filelist = [];
   return; 
end
for k=1:nfn
    filelist(k) = {fn(k).name};
end
return

function plot_refl(fnlst)
figure(1);clf
figure(2);clf
figure(3);clf
for i=1:length(fnlst)
   show_refl(char(fnlst(i)),i);
   drawnow
end
figure(1)
subplot(2,1,1);hold off
subplot(2,1,2);hold off
figure(2);hold off
figure(3);hold off
return
%
function show_refl(fn,seq)
global fmax xmax T
if (isoctave)
   load(fn);    		% fetch data
else
   load(fn,'-MAT');	% fetch data
end
fmax=min([fmax rate/2000]);
zl=[zl;mean(zl)];
pl=[pl;mean(pl)];
%
lc=cavity_length(pl,rate)*10;
fprintf('%s: length = %4.1f\n',fn,lc);
zo=surge(zl);
pr=(1+zo./zl)/2;
zl=zl/zo;
n=length(pr)-1;
f=(0:(n))'*df/1000;
f1=0.1;
f2=15;
j=find(f>f1&f<f2);
fj=f(j);
nj=length(j);
%
figure(1)
dblim=[-6.5 0.5];
phlim=[-6 1];
gdlim=[-0.2 1];
R=(zl-1)./(zl+1);
db=20*log10(abs(R)+eps);
ph=unwrap(angle(R))/(2*pi);
gd=delay(R,f);
subplot(2,1,1);
plot(f,db)
axis([0 fmax dblim]);
ylabel('magnitude (dB)')
title('frequency-domain reflectance');
hold on
subplot(2,1,2);
plot(f,gd)
axis([0 fmax gdlim]);
ylabel('delay (ms)')
xlabel('frequency (kHz)')
hold on
%
figure(2)
R=(zl-1)./(zl+1);
mf=4;
r=ffs(fd_window(R,mf));
r=fftshift(r);
n=length(r)/2;
t=((-n):(n-1))*1000/rate/mf;
lim=[-0.2 1 -0.05 0.45];
plot(t,r)
axis(lim)
title('time-domain reflectance');
xlabel('time (ms)')
hold on
r=fftshift(r);
%
figure(3)
sr=rate*mf;
[A,B,x]=refl_inverse(r,zo,sr,xmax,T);
D=20*sqrt(A/pi);
x=x*10;
lim=[-5 95 -0.5 10.5];
plot(x,D,'r')
axis(lim)
title('inverse solution');
xlabel('distance (mm)')
ylabel('diameter (mm)')
hold on
%
return
%
% inverse solution for radius from reflectance
function [A,B,x]=refl_inverse(r,zo,rate,xmax,T);
rho = 1.1769e-3 * (1 - 0.00335 * (T - 26.85));
c   = 3.4723e4  * (1 + 0.00166 * (T - 26.85));
dt=1/rate;
dx=c*dt;
Ao=rho*c/zo;
m=ceil(xmax/dx);
if (length(r)<(2*m))
   m=floor(length(r)/2);
   xmax=m*dx;
   fprintf('reduced xmax=%5.2f\n',xmax);
end
x=(0:(m-1))*dx;
epmx=1/dx;						% maximum epsilon
etmx=1/dx;						% maximum eta
pfmx=1.01; 						% maximum power flow
pfmn=-0.01;						% minimum power flow
%
n=2*m;
pm=r(1:n);
pp=zeros(n,1);
pp(1)=1;
pm(1)=0;
A=zeros(1,m);
B=zeros(1,m);
A(1)=Ao;
for k=1:(m-1)
   p=pp+pm;						% pressure
   q=cumsum(p)*dt;			% integrated pressure
   u=pp-pm+c*B(k)*q;			% velocity / Yo
   a=A(k)/Ao;					% relative area
   pf=p(k)*conj(u(k))*a; 	% power flow
   if ((pf>pfmx)|(pf<pfmn)) break; end	% check #1
   j1=k;
   j2=k+1;
   bk=B(k)/2;
   v1=bk*(p(j1)+u(j1))-pm(j1)/dx;
   v2=bk*(p(j2)+u(j2))-pm(j2)/dx;
   dd=u(j1)*q(j2)-q(j1)*u(j2);
   if (dd==0) break; end					% check #2
   ep=(q(j2)*v1-q(j1)*v2)/dd;
   et=(u(j1)*v2-u(j2)*v1)/dd;
   if ((ep>epmx)) break; end				% check #3
   if ((et>etmx)&(bk<-1)) break; end	% check #4
   npp=zeros(n,1);
   npm=zeros(n,1);
   for j=(k+1):(n-k)
      jp=j+1;
      jm=j-1;
      vp=bk*p(jp)-((ep-bk)*u(jp)+et*q(jp));
      vm=bk*p(jm)+((ep-bk)*u(jm)+et*q(jm));
      npp(j)=pp(jm)-vm*dx;
      npm(j)=pm(jp)-vp*dx;
   end
   pp=npp;
   pm=npm;
   A(k+1)=A(k)*exp(2*ep*dx);
   B(k+1)=B(k)+2*et*dt;
end
if (0)
   fprintf('x=%5.2f A=%6.3g B=%6.3g ',x(k),A(k),B(k));
   fprintf('ep=%6.3g et=%6.3g pf=%6.3g\n',ep,et,pf);
end;
return

function [lk,td]=cavity_length(pc,rate)
global T
lmn=1;      % minimum length (cm)
lmx=12;     % maximum length (cm)
c=3.4723e4*(1+0.00166*(T-26.85));
%
imn=round(lmn*rate*2/c)+1;
imx=round(lmx*rate*2/c)+1;
p=ffs(abs(pc).^2);
[x,m]=max(p(imn:imx));m=m+imn-1;
d =(p(m-1)-p(m+1))/(p(m-1)-2*p(m)+p(m+1))/2;
td =(m+d-1)/rate;
lk=td*c/2;
return

function H=fd_window(H,mf)
global fdw
if (fdw)
   n=length(H)-1;
   p=pi*(0:n)'/n;
   a=0.16; % Blackman window
   w=(1-a+cos(p)+a*cos(2*p))/2;
   H=H.*w;
   H=[H;zeros((mf-1)*n+1,1)]*mf;
end
return

function zo=surge(zl)
zo=mean(real(zl));
wo=mean(fd_window(ones(size(zl)),1));
for k=1:4
    wr=mean(real(fd_window((zl-zo)./(zl+zo),1)));
    zo=zo*(1+wr/wo);
 end
return
%
% fast Fourier synthesize real signal
function h=ffs(H)
m=length(H);
n=2*(m-1);
H(1,:)=real(H(1,:));
H(m,:)=real(H(m,:));
H((m+1):n,:)=conj(H((m-1):-1:2,:));
h=real(ifft(H));
return
%
% group delay
function gd=delay(R,f)
[n,m] = size(R);
ph = unwrap(angle(R))/(2*pi);
gd = zeros([n,m]);
for k=1:m
   gd(:,k) = -cdif(ph(:,k))./cdif(f(:));
end
return
%
% centered difference
function dx=cdif(x)
n=length(x);
dx=zeros(size(x));
dx(1)=x(2)-x(1);
dx(2:(n-1))=(x(3:n)-x(1:(n-2)))/2;
dx(n)=x(n)-x(n-1);
return

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function o = isoctave
o=1;eval('OCTAVE_VERSION;','o=0;');

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
function write_data(fn,data)
[nr,nc] = size(data);
fp=fopen(fn,'wt');
fprintf(fp,'; %s\n', fn);
for i=1:nr
    for j=1:nc;
        fprintf(fp,' %14.5g',data(i,j));
    end
    fprintf(fp,'\n');
end
fclose(fp);
