// https://stackoverflow.com/questions/35923787/fast-large-matrix-multiplication-in-r
// [[Rcpp::depends(RcppArmadillo)]]

#include <RcppArmadillo.h>
#include <Rcpp.h>

arma::mat MatMultAtBC(arma::mat A, arma::mat B, arma::mat C){
    arma::mat At = A.t();
    arma::mat D = At * B * C;
    return(D);
}

arma::mat predict(arma::mat x, arma::mat W, arma::vec b){
    arma::mat y = x * W * W.t();
    y.each_row() += b.t();
    return(y);
}

// Col-wise inplace elemtent-wise multiplication
arma::mat sexpyfun(arma::mat y, arma::vec s){
    arma::mat sexpy = arma::exp(y);
    sexpy.each_col() %= s;
    return(sexpy);  
} 


double computeLoss(arma::mat k, arma::mat y, arma::vec s, arma::vec theta){
    
    arma::mat sexpy = sexpyfun(y, s);
    
    //compute log(s) + y
    arma::vec logs = arma::log(s);
    y.each_col() += logs;

    double m1=-arma::accu(k % y);
    double m2= arma::accu((k.each_row() + theta) % arma::log(sexpy.each_row() + theta));
    return(m1+m2);
}


arma::mat computeKT(arma::mat k, arma::mat x, arma::mat W, arma::vec b, arma::vec s, arma::vec theta){
    arma::mat y = predict(x, W, b);
    arma::mat sexpy = sexpyfun(y, s);
    arma::mat kt = (k.each_row() + theta)%sexpy/(sexpy.each_row() + theta);
    return(kt);
}


// [[Rcpp::export(rng=false)]]
SEXP truncLogLiklihood(arma::mat k, arma::mat x, arma::mat W, arma::vec b, 
                    arma::vec s, arma::vec theta){
    arma::mat y = predict(x, W, b);
    double Loss = computeLoss(k, y, s, theta);
    Loss /= k.n_elem;
    return Rcpp::wrap(Loss);
}


// [[Rcpp::export(rng=false)]]
SEXP gradLogLiklihood(arma::mat k, arma::mat x, arma::mat W, arma::vec b, 
                    arma::vec s, arma::vec theta){
    
    arma::mat kt = computeKT(k, x, W, b, s, theta);
    
    arma::mat t1 = MatMultAtBC(x, k, W);
    arma::mat t2 = MatMultAtBC(k, x, W);
    
    arma::mat t3 = MatMultAtBC(x, kt, W);
    arma::mat t4 = MatMultAtBC(kt, x, W);
    
    arma::mat dw = (-t1 - t2 + t3 + t4);
    arma::mat db = arma::sum(kt-k,0).t();
    arma::mat grad = join_rows(dw, db);
    grad /= k.n_elem;
    return Rcpp::wrap(grad);
}


