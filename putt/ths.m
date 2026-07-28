% ths.m - Show contents of PUTT THS file.
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
show_ths(fn,1)
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

function show_ths(fn,fig)
fprintf('%s\n',fn);
if (isoctave)
   load(fn);   		% fetch data
else
   load(fn,'-MAT');	% fetch data
end
n=length(ps);
f=(0:(n-1))*df;
%
show_zp(f,zs,ps,fig,fn,'source')     % plot zs & ps
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
xlabel('frequency (kHz)')
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
return

