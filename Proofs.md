## Theorem 1



$ {\rm{Z}}_p^{{\rm{|}}{{P}_{p,q}}{\rm{|}}}{\rm{ = Z}}_q^{{\rm{|}}{{P}_{p,q}}{\rm{|}}} $ and $ t_p^{|{{P}_{p,q}}{\rm{| + }}1}{\rm{ = }}t_q^{|{{ P}_{p,q}}{\rm{| + }}1} $



**Proof:** It is obvious that if $|{P}_{p,q}|=0$, which means $p$ and $q$ have no common levels, then ${{Z}_p^0}={{Z}_q^0}$ and $t_p^1 = t_q^1$ holds.

For ${|{P}_{p,q}|}\geq 1$, which means $p$ and $q$ have ${|{P}_{p,q}|}$ common levels. In this case, for $1\leq i \leq {|{P}_{p,q}|}$, as $x_p^i=x_q^i$, $y_p^i=y_q^i$ and $t_p^i = t_q^i$, we always have $z_p^{2i-1}z_p^{2i}=z_q^{2i-1}z_q^{2i}$, $Z_p^i=Z_q^i$ and $t_p^{i+1} = t_q^{i+1}$.

Thus, Theorem 1 holds.



### ## Preposition 1



The AEL of NA-HE is less than 2 when encoding a 𝑘-level window.



**Proof:** We first compute the total encoding levels (TEL) for a $k$-level window. To do this, we divide all the coordinates in the window into three subsets: A (the first coordinate in the first line), B(the first encoded coordinates of all lines along S-order except the first line), C(all coordinates not in A and B). For ease of presentation, we denote $E_S$ as the TEL for a subset S of the three subsets.

For A, since it contains the first coordinate only, all the $k$ levels need to be encoded, we have $E_A$ = $k$.

For B, notice that the TELs for all coordinates in $B$ form a \textbf{recursive centrosymmetric sequence} with a center $k$, where the left part and right part of this sequence are all recursive centrosymmetric sequences with a center $k-1$. There are exactly one $k$, two $(k-1)$s,..., $2^{k-1}$ 1s in this sequence. Therefore, ${E_B}{\rm{ = }}\sum\limits_{j = 1}^k{j}$ $\times{2^{k - j}}$  ${\rm{ = }}{2^{k{\rm{ + }}1}}{\rm{ - }}k{\rm{ - 2}}$.

For C, the TELs for all coordinates in $C_i$ is the same recursive centrosymmetric sequence as B, where $C_i$ denotes the set of coordinates in $C$ and in the $i$-th line of the window. Therefore, for all the $2^k$ lines, we have ${E_C} = {2^k}\times({2^{k{\rm{ + }}1}}{\rm{ - }}k{\rm{ - 2)}}$.

Thus, the TEL is $E_A$ + $E_B$ + $E_C$ = $2^{2k + 1} - k \times 2^k - 2$ and the AEL is $\frac{2^{2k + 1} - k\times{2^k} - 2}{2^{2k}} < 2$ for a $k$-level window. Preposition 1 holds.