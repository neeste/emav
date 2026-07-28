% refl_td.m - Show reflectance from EMAV THL files
%
function refl_td
global fmax xmax T fdw wd
fmax=20;    % maximum frequency (kHz)
xmax=12;    % maximum tube length (cm)
T=25;			% air temperature (deg C)
fdw=1;      % frequency-domain window ?
wd=1;       % write data ?
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
subplot(2,2,1);hold off
subplot(2,2,2);hold off
subplot(2,2,3);hold off
subplot(2,2,4);hold off
figure(2)
subplot(1,2,1);hold off
subplot(1,2,2);hold off
figure(3)
subplot(1,2,1);hold off
subplot(1,2,2);hold off
return

function show_refl(fn,seq)
global fmax xmax T wd
if (isoctave)
   load(fn);    		% fetch data
else
   load(fn,'-MAT');	% fetch data
end
fmax=min([fmax rate/2000]);
zl1=[zl1;mean(zl1)];
zl2=[zl2;mean(zl2)];
pl1=[pl1;mean(pl1)];
pl2=[pl2;mean(pl2)];
%
n=length(zl1)-1;
f=(0:n)'*df/1000;
l1=cavity_length(pl1,rate)*10;
l2=cavity_length(pl2,rate)*10;
zo1=surge(zl1);
zo2=surge(zl2);
fprintf('%s: length = %4.1f %4.1f; surge = %4.1f %4.1f\n',fn,l1,l2,zo1,zo2);
R1=(zl1-zo1)./(zl1+zo1);
R2=(zl2-zo2)./(zl2+zo2);
%
figure(1)
dblim=[-6.5 0.5];
phlim=[-6 1];
gdlim=[-0.2 1];
db1=20*log10(abs(R1)+eps);
db2=20*log10(abs(R2)+eps);
ph1=unwrap(angle(R1))/(2*pi);
ph2=unwrap(angle(R2))/(2*pi);
gd1=delay(R1,f);
gd2=delay(R2,f);
subplot(2,2,1);
plot(f,db1,'r')
axis([0 fmax dblim]);
ylabel('magnitude (dB)')
title('fd reflectance - 1');
hold on
subplot(2,2,3);
plot(f,gd1,'r')
axis([0 fmax gdlim]);
ylabel('delay (ms)')
xlabel('frequency (kHz)')
hold on
%
subplot(2,2,2);
plot(f,db2,'b')
axis([0 fmax dblim]);
ylabel('magnitude (dB)')
title('fd reflectance - 2');
hold on
subplot(2,2,4);
plot(f,gd2,'b')
axis([0 fmax gdlim]);
ylabel('delay (ms)')
xlabel('frequency (kHz)')
hold on
if (wd)
   write_data(sprintf('rf1_%d.txt',seq),[f db1 gd1]);
   write_data(sprintf('rf2_%d.txt',seq),[f db2 gd2]);
end
drawnow
%
figure(2)
mf=4;
r1=ffs(fd_window(R1,mf));
r2=ffs(fd_window(R2,mf));
r1=fftshift(r1);
r2=fftshift(r2);
n=length(r1)/2;
t=((-n):(n-1))*1000/rate/mf;
lim=[-0.2 1 -0.05 0.45];
subplot(1,2,1);
plot(t,r1,'r')
axis(lim)
title('td reflectance - 1');
xlabel('time (ms)')
hold on
subplot(1,2,2);
plot(t,r2,'b')
axis(lim)
title('td reflectance - 2');
xlabel('time (ms)')
hold on
if (wd)
   write_data(sprintf('rt1_%d.txt',seq),[t(:) r1(:)]);
   write_data(sprintf('rt2_%d.txt',seq),[t(:) r2(:)]);
end
drawnow
r1=fftshift(r1);
r2=fftshift(r2);
%
figure(3)
sr=rate*mf;
[A1,B1,x]=refl_inverse(r1,zo1,sr,xmax,T);
[A2,B2,x]=refl_inverse(r2,zo2,sr,xmax,T);
D1=20*sqrt(A1/pi);
D2=20*sqrt(A2/pi);
x=x*10;
lim=[-5 95 -0.5 10.5];
subplot(1,2,1);
plot(x,D1,'r')
axis(lim)
title('inverse solution - 1');
xlabel('distance (mm)')
ylabel('diameter (mm)')
hold on
subplot(1,2,2);
plot(x,D2,'b')
axis(lim)
title('inverse solution - 2');
xlabel('distance (mm)')
hold on
if (wd)
   write_data(sprintf('dx1_%d.txt',seq),[x(:) D1(:)]);
   write_data(sprintf('dx2_%d.txt',seq),[x(:) D2(:)]);
end
drawnow
%
return

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

% group delay
function gd=delay(R,f)
[n,m] = size(R);
ph = unwrap(angle(R))/(2*pi);
gd = zeros([n,m]);
for k=1:m
   gd(:,k) = -cdif(ph(:,k))./cdif(f(:));
end
return

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
