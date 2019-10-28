# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

# Proprietary Information D-Wave Systems Inc.
# Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
# Notice this code is licensed to authorized users only under the
# applicable license agreement see eula.txt
# D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

from collections import defaultdict
from copy import deepcopy
from math import log


def chimera_to_linear_index(*args):
    """
    [linear_index0, linear_index1, ...] = chimera_to_linear_index(i, j, u, k, m, n, t)

    Convert Chimera adjacency matrix indices i, j, u, k into linear indices.
    m, n, t give the size of the Chimera graph. The lattice is MxN
    and each bipartite component is K_{t, t}. i, j, u, k may be column
    vectors of a common length and we return the linear index for each
    element. i, j label the lattice location with 0 <= i <= m - 1 and 0 <= j <= n - 1,
    0 <= u <= 1 labels which partition, and 0 <= k <= t - 1 the vertex within a
    partition. See linear_index_to_chimera(t) for the inverse function.

    [linear_index0, linear_index1, ...] = chimera_to_linear_index(idx, m, n, t)
    As above but idx is an n x 4 list of indices

    Args:
        If seven parameters provided, param0: a list or tuple of i
                                      param1: a list or tuple of j
                                      param2: a list or tuple of u
                                      param3: a list or tuple of k
                                      param4: m for Chimera graph
                                      param5: n for Chimera graph
                                      param6: t for Chimera graph

        If four parameters provided, param0: a list of list or a tuple
                                     of tuple, each list or tuple contained
                                     must have four elements representing
                                     i, j, u, k
                                     param1: m for Chimera graph
                                     param2: n for Chimera graph
                                     param3: t for Chimera graph

    Returns:
        A list of linear indices

    Raises:
        ValueError: bad parameter value
    """
    if len(args) == 7:
        chimera_indices = zip(*args[:4])
        m, n, t = args[4:]
    else:
        chimera_indices, m, n, t = args

    hf = 2 * t
    vf = n * hf
    linear_indices = [cr * vf + cc * hf + cu * t + ck
                      for cr, cc, cu, ck in chimera_indices]
    return linear_indices


def linear_index_to_chimera(linear_indices, m, n=None, t=None):
    """
    [[i0, j0, u0, k0], [i1, j1, u1, k1], ...] = linear_index_to_chimera(linear_indices, m, n, t)

    Convert linear indices to indices (i, j, u, k) into the Chimera adjacency
    matrix. m, n, t give the size of the Chimera graph. The lattice is m x n
    and each bipartite component is K_{t, t}. i, j, u, k may be column
    vectors of a common length and we return the linear index for each
    element. i, j label the lattice location with 0 <= i <= m - 1 and 0 <= j <= n - 1,
    0 <= u <= 1 labels the partition, and 0 <= k <= t - 1 the vertex within a
    partition. See chimera_to_linear_index for the inverse function.

    Args:
        linear_indices: a list or tuple of linear indices
        m: m for Chimera graph
        n: n for Chimera graph
        t: t for Chimera graph

    Returns:
        A list of list, each list contained has four elements representing
        i, j, u, k

    Raises:
        ValueError: bad parameter value
    """
    if n is None:
        n = m
    if t is None:
        t = 4

    hd = 2 * t
    vd = n * hd
    chimera_indices = []
    for ti in linear_indices:
        r, ti = divmod(ti, vd)
        c, ti = divmod(ti, hd)
        u, k = divmod(ti, t)
        chimera_indices.append([r, c, u, k])
    return chimera_indices


def get_chimera_adjacency(m, n=None, t=None):
    """
    A = get_chimera_adjacency(m, n, t)

    Build the adjacency matrix A for the Chimera architecture. The
    architecture is an m x n lattice of elements where each element is a
    K_{l, l} bipartite graph. In this representation the rows and columns
    of A are indexed by i, j, u, k where
       0 <= i <= m - 1 gives the row in the lattice
       0 <= j <= n - 1 gives the column in the lattice
       0 <= u <= 1 labels which of the two partitions
       0 <= k <= t - 1 labels one of the L elements in the partition
    Matrix elements of A are explicitly given by
      A_{i, j; u; k | i', j'; u'; k'} =
          [i == i'] [j == j'] [|u - u'| == 1] +            (bipartite connections)
          [|i - i'| == 1] [j == j'] [u == u' == 0] [k == k'] + (row connections)
          [i == i'] [|j - j'| == 1] [u == u' == 1] [k == k']   (col connections)
      where [p] = 1 if p is true and [p] = 0 if p is false

    A = get_chimera_adjacency(m, n)
    As above but with the default value t = 4.

    A = get_chimera_adjacency(m)
    As above but with the default value t = 4 and n = m.

    Args:
        m: M for Chimera graph
        n: N for Chimera graph
        l: L for Chimera graph

    Returns:
        A set contains Chimera adjancency edges

    Raises:
        ValueError: bad parameter value
    """
    if n is None:
        n = m
    if t is None:
        t = 4

    hoff = 2 * t
    voff = n * hoff
    mi = m * voff
    ni = n * hoff

    # cell edges
    adj = set((k0, k1)
              for i in xrange(0, ni, hoff)
              for j in xrange(i, mi, voff)
              for k0 in xrange(j, j + t)
              for k1 in xrange(j + t, j + 2 * t))
    # horizontal edges
    adj.update((k, k + hoff)
               for i in xrange(t, 2 * t)
               for j in xrange(i, ni - hoff, hoff)
               for k in xrange(j, mi, voff))
    # vertical edges
    adj.update((k, k + voff)
               for i in xrange(t)
               for j in xrange(i, ni, hoff)
               for k in xrange(j, mi - voff, voff))

    return adj | set(c[::-1] for c in adj)


def get_hardware_adjacency(solver):
    """
    A = get_hardware_adjacency(solver)

    Build the adjacency matrix A for the solver.

    Args:
        solver: a sapi solver

    Returns:
        A set contains solver's adjancency edges

    Raises:
        KeyError: solver's properties doesn't have key: couplers
    """
    adj = set(tuple(c) for c in solver.properties['couplers'])
    return adj | set(c[::-1] for c in adj)


def ising_to_qubo(h, j):
    """
    (Q, offset) = ising_to_qubo(h, j)

    Map an ising model defined over -1/+1 variables to a binary quadratic
    program x' * Q * x defined over 0/1 variables. We return the Q defining
    the BQP model as well as the offset in energy between the two problem
    formulations, i.e. s' * J * s + h' * s = offset + x' * Q * x. The linear term
    of the BQP is contained along the diagonal of Q.

    See qubo_to_ising(Q) for the inverse function.

    Args:
        h: h for ising problem
        j: J for ising problem

    Returns:
        A tuple which contains a dictionary Q for qubo problem and offset

    Raises:
        ValueError: bad parameter value
    """

    q = defaultdict(float)
    offset = 0
    for i, e in enumerate(h):
        q[(i, i)] = 2 * e
        offset -= e
    for (i, j), e in j.iteritems():
        q[(i, j)] += 4 * e
        q[(i, i)] -= 2 * e
        q[(j, j)] -= 2 * e
        offset += e
    q = dict((k, v) for k, v in q.iteritems() if v != 0)
    return q, offset


def qubo_to_ising(q):
    """
    (h, J, offset) = qubo_to_ising(q)

    Map a binary quadratic program x' * Q * x defined over 0/1 variables to
    an ising model defined over -1/+1 variables. We return the h and J
    defining the Ising model as well as the offset in energy between the
    two problem formulations, i.e. x' * Q * x = offset + s' * J * s + h' * s. The
    linear term of the qubo is contained along the diagonal of Q.

    See ising_to_qubo(h, J) for the inverse function.

    Args:
        q: Q for qubo problem

    Returns:
        A tuple which contains a list h for ising problem, a dictionary J
        for ising problem and offset

    Raises:
        ValueError: bad parameter value
    """
    hdict = defaultdict(float)
    j = {}
    offset = 0
    for (i, k), e in q.iteritems():
        if i == k:
            hdict[i] += 0.5 * e
            offset += 0.5 * e
        else:
            j[(i, k)] = 0.25 * e
            hdict[i] += 0.25 * e
            hdict[k] += 0.25 * e
            offset += 0.25 * e
    hdict = dict((k, v) for k, v in hdict.iteritems() if v != 0)
    if hdict:
        h = [0] * (1 + max(hdict))
    else:
        h = []
    for i, v in hdict.iteritems():
        h[i] = v
    j = dict((k, v) for k, v in j.iteritems() if v != 0)
    return h, j, offset


def reduce_degree(terms):
    """
    (terms, mapping) = reduce_degree(terms)

    Reduce the degree of a set of objectives specified by terms to have maximum two
    degrees via the introduction of ancillary variables.

    Args:
        terms: list of list representing each term's variables in the expression

    Returns:
        new_terms: terms after using ancillary variables
        mapping: list of ancillary variables

    Raises:
        ValueError
    """
    terms = deepcopy(terms)
    pair_belongs_to = dict()
    max_var = 0
    for i in range(len(terms)):
        for ii in range(len(terms[i])):
            if terms[i][ii] > max_var:
                max_var = terms[i][ii]
        if len(terms[i]) > 2:
            for j in range(len(terms[i])):
                for k in range(j + 1, len(terms[i])):
                    tmp_1 = min(terms[i][j], terms[i][k])
                    tmp_2 = max(terms[i][j], terms[i][k])
                    if (tmp_1, tmp_2) not in pair_belongs_to:
                        pair_belongs_to[(tmp_1, tmp_2)] = set()
                    pair_belongs_to[(tmp_1, tmp_2)].add(i)

    mapping = []
    while len(pair_belongs_to) != 0:
        max_v_len = 0
        max_k = ()
        for k, v in pair_belongs_to.iteritems():
            if len(v) > max_v_len:
                max_v_len = len(v)
                max_k = k

        max_var += 1
        mapping.append([max_var, max_k[0], max_k[1]])
        affected = deepcopy(pair_belongs_to[max_k])
        for k in affected:
            for j in range(len(terms[k])):
                for t in range(j + 1, len(terms[k])):
                    tmp_1 = min(terms[k][j], terms[k][t])
                    tmp_2 = max(terms[k][j], terms[k][t])
                    pair_belongs_to[(tmp_1, tmp_2)].remove(k)
                    if len(pair_belongs_to[(tmp_1, tmp_2)]) == 0:
                        del pair_belongs_to[(tmp_1, tmp_2)]
            terms[k].remove(max_k[0])
            terms[k].remove(max_k[1])
            terms[k].append(max_var)

            if len(terms[k]) > 2:
                for j in range(len(terms[k])):
                    for t in range(j + 1, len(terms[k])):
                        tmp_1 = min(terms[k][j], terms[k][t])
                        tmp_2 = max(terms[k][j], terms[k][t])
                        if (tmp_1, tmp_2) not in pair_belongs_to:
                            pair_belongs_to[(tmp_1, tmp_2)] = set()
                        pair_belongs_to[(tmp_1, tmp_2)].add(k)

    return terms, mapping


def make_quadratic(f, penalty_weight=None):
    """
    (Q, new_terms, mapping) = make_quadratic(f, penalty_weight=10)

    Given a function f defined over binary variables represented as a
    vector stored in decimal order, e.g. f[000], f[001], f[010], f[011],
    f[100], f[101], f[110], f[111] represent it as a quadratic objective.

    Args:
        f: a function defined over binary variables represented as a
        list stored in decimal order, f's length must be power of 2
        penalty_weight: if this parameter is supplied it sets the strength of the
        penalty used to define the product constaints on the new ancillary
        variables. The default value is usually sufficiently large, but may be
        larger than necessary.

    Returns:
        Q: quadratic coefficients
        new_terms: the terms in the QUBO arising from quadraticization of the
        interactions present in f
        mapping: the definition of the new ancillary variables, mapping(i, 0)
        represents the product of mapping(i, 1) and mapping(i, 2)

    Raises:
        ValueError
    """
    # parameter checking
    n = len(f)

    if not (n > 0 and (n & (n - 1)) == 0):
        raise ValueError("f's length is not power of 2")

    if f[0] != 0:
        raise ValueError(
            "make_quadratic expects f[0], the constant to be zero")

    # find coefficients
    num_vars = int(log(n, 2))

    inv_w = [None] * n
    for i in range(n):
        inv_w[i] = [0] * n

    inv_w[0][0] = 1
    if num_vars >= 1:
        inv_w[1][0] = -1
        inv_w[1][1] = 1
    for i in range(1, num_vars):
        tmp = 2 ** i
        for j in range(tmp):
            for k in range(tmp):
                inv_w[j + tmp][k] = - inv_w[j][k]
                inv_w[j + tmp][k + tmp] = inv_w[j][k]

    c = [0] * n
    for i in range(n):
        for j in range(n):
            c[i] += inv_w[i][j] * f[j]

    # find non-zero coefficients
    terms = []
    non_zero_index = []
    for i in range(n):
        if abs(c[i]) > 1e-10:
            non_zero_index.append(i)
            tmp = []
            for j in range(num_vars):
                if i & (1 << j):
                    tmp.append(j)
            terms.append(tmp)

    # find interactions using as few new variables as possible
    new_terms, mapping = reduce_degree(terms)

    # add in to the objective
    new_num_vars = num_vars + len(mapping)
    q = dict()
    r = [0] * new_num_vars
    for i in range(len(new_terms)):
        if len(new_terms[i]) == 1:
            r[new_terms[i][0]] += c[non_zero_index[i]]
        else:
            if (new_terms[i][0], new_terms[i][1]) not in q:
                q[new_terms[i][0], new_terms[i][1]] = 0
            if (new_terms[i][1], new_terms[i][0]) not in q:
                q[new_terms[i][1], new_terms[i][0]] = 0
            q[(new_terms[i][0], new_terms[i][1])] = q[(
                new_terms[i][0], new_terms[i][1])] + c[non_zero_index[i]] / 2.0
            q[(new_terms[i][1], new_terms[i][0])] = q[(
                new_terms[i][1], new_terms[i][0])] + c[non_zero_index[i]] / 2.0

    if penalty_weight is None:
        penalty_weight = 0.0
        for k, v in q.iteritems():
            penalty_weight = max(penalty_weight, abs(v))
        penalty_weight *= 10

    # now add the penalty terms enforcing pairwise constraints
    if new_num_vars > num_vars:
        q_and = [[0.0, -penalty_weight, -penalty_weight],
                 [-penalty_weight, 0.0, penalty_weight / 2.0],
                 [-penalty_weight, penalty_weight / 2.0, 0.0]]
        r_and = [3.0 * penalty_weight, 0.0, 0.0]
        for k in range(len(mapping)):
            for i in range(len(mapping[k])):
                for j in range(len(mapping[k])):
                    if (mapping[k][i], mapping[k][j]) not in q:
                        q[(mapping[k][i], mapping[k][j])] = 0
                    q[(mapping[k][i], mapping[k][j])] = q[(
                        mapping[k][i], mapping[k][j])] + q_and[i][j]
                r[mapping[k][i]] += r_and[i]

    for i in range(len(r)):
        if (i, i) not in q:
            q[(i, i)] = 0
        q[(i, i)] += r[i]

    return q, new_terms, mapping
