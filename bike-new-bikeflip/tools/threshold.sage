# Pre-compute the ideal threshold as an approximate affine function of
# the syndrome weight

def rho(i, r, d, n, w, t):
    return float(binomial(w,i) * binomial(n-w,t-i) / binomial(n,t))

def E(i, r, d, n, w, t):
    return r * rho(i, r, d, n, w, t)

def pi1(S, tau, r, d, n, w, t):
    return S*(1+tau)/d/t

def pi0(S, tau, r, d, n, w, t):
    return S*(w-1-tau)/d/(n-t)

def threshold(S, r, d, n, w, t):
    tau = sum(2*i*rho(2*i+1, r, d, n, w, t) for i in range(6)) / sum(rho(2*i+1, r, d, n, w, t) for i in range(6))
    p0, p1 = pi0(S, tau, r, d, n, w, t), pi1(S, tau, r, d, n, w, t)
    return float((log(t/(n-t)) + d*log((1-p1)/(1-p0)))/log(p0*(1-p1)/p1/(1-p0)))

# the threshold is viewed as a function of the syndrome weight
# the linear approximation is the tangent at the average syndrome weight
def linear_approx(r, d, t):
    w, n = d * 2, r * 2
    S = floor(sum(rho(2*i+1, r, d, n, w, t) for i in range(6))*r)
    T = threshold(S, r, d, n, w, t)
    a = threshold(S + 1, r, d, n, w, t) - T
    b = T - a * S
    return (a, b)

def print_threshold_header(r, d, t):
    w, n = d * 2, r * 2
    a, b = linear_approx(r, d, t)
    S = floor(sum(rho(2*i+1, r, d, n, w, t) for i in range(6))*r)
    print("// Threshold function for BIKE parameters (r, d, t) = (%d, %d, %d)" % (r, d, t))
    print("#define VAR_TH_FCT(x) (" + str(b) + " + " + str(a) + " * (x))")

print_threshold_header(12323,71,134)

print_threshold_header(24659,103,199)

print_threshold_header(40973,137,264)
