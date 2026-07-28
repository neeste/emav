% thverr - calculate error between measured and caclulated pressure
function err = thverr(pa)
global xtk pj fj
f = fj;                     % reduce frequency range
pc = pj;                    % reduce frequency range of pc
[zc,z0] = cavimp(f,pa);     % calculate cavity impedances
[zs,ps] = thvsrc(zc,pc);    % estimate zs & ps
[nf,nc] = size(pc);         % number of cavities
s1 = 0;
s2 = 0;
pa = zeros(nf,1);
for k=1:nc
   pd = pc(:,k) - ps .* zc(:,k) ./ (zs + zc(:,k));
   pa = pa + pd;
   s1 = s1 + sum(abs(pd).^2);
   s2 = s2 + sum(abs(pc(:,k)).^2);
end
if (xtk)
    s1 = s1 - sum(abs(pa).^2) / nc;
end
err = 1e4 * s1 / s2;
return

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

