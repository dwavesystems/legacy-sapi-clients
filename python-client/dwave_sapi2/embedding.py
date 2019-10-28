# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

# Proprietary Information D-Wave Systems Inc.
# Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
# Notice this code is licensed to authorized users only under the
# applicable license agreement see eula.txt
# D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

from __future__ import division

import heapq
from random import choice, random

from find_embedding_impl import find_embedding_impl


def find_embedding(Q, A, **params):
    """Attempts to find an embedding of a QUBO/Ising problem in a graph.

    This function is entirely heuristic: failure to return an embedding
    does not prove that no embedding exists. (can be interrupted by
    Ctrl-C, will return the best embedding found so far.)

    Function call:
       embeddings = find_embedding(Q, A, **params)

    Args:
        Q: Edge structures of a problem. The embedder only cares about
            the edge structure (i.e. which variables have a nontrivial
            interactions), not the coefficient values. (must be an
            iterable object containing pairs of integers representing
            edges)

        A: Adjacency matrix of the graph. (must be an iterable object
            containing pairs of integers representing edges)

        **params: keyword parameters for find_embedding (if the
            following keyword parameter is set as None, it will use its
            default value):

            fast_embedding: True/False, tries to get an embedding
                quickly, without worrying about chain length. (must be a
                boolean, default = False)

            max_no_improvement: number of rounds of the algorithm to try
                from the current solution with no improvement. Each
                round consists of an attempt to find an embedding for
                each variable of Q such that it is adjacent to all its
                neighbours. (must be an integer >= 0, default = 10)

            random_seed: seed for random number generator that
                find_embedding uses (must be an integer >= 0, default is
                randomly set)

            timeout: Algorithm gives up after timeout seconds. (must be
                a number >= 0, default is approximately 1000 seconds)

            tries: The algorithm stops after this number of restart
                attempts. (must be an integer >= 0, default = 10)

            verbose: 0/1. (must be an integer [0, 1], default = 0) when
                verbose is 1, the output information will be like:
                    try ...
                    max overfill = ..., num max overfills = ...
                    Embedding found. Minimizing chains...
                    max chain size = ..., num max chains = ..., qubits used = ...
                detailed explanation of the output information:
                    try: ith (0-based) try
                    max overfill: largest number of variables represented in a qubit
                    num max overfills: the number of qubits that has max overfill
                    max chain size: largest number of qubits representing a single variable
                    num max chains: the number of variables that has max chain size
                    qubits used: the total number of qubits used to represent variables

    Returns:
        embeddings: A list of lists of embeddings. embeddings[i] is the
            list of qubits representing logical variable i. This
            embeddings return value can be used in EmbeddingSolver. If
            the algorithm fails, the output is an empty list."""
    return find_embedding_impl(Q, A, params)


def _are_connected(emb, adj):
    visit = [next(iter(emb))]
    rem = set(emb).difference(visit)
    while visit:
        u = visit.pop()
        new = [v for v in rem if (u, v) in adj]
        visit.extend(new)
        rem.difference_update(new)
    return not rem


def _verify_embedding_1(embeddings, adj):
    used = set()
    for i, emb in enumerate(embeddings):
        if not emb:
            raise ValueError("logical variable %d is empty" % i)
        if any(u in used for u in emb):
            raise ValueError('embeddings are not disjoint')
        bad = [u for u in emb if (u, u) not in adj]
        if bad:
            raise ValueError('invalid embedded variable %d in logical '
                             'variable %d' % (bad[0], i))
        if not _are_connected(emb, adj):
            raise ValueError("embedding of logical variable %d is "
                             "disconnected" % i)


def _verify_embedding_2(embeddings, j, adj):
    for (i0, i1), v in j.iteritems():
        if all((x, y) not in adj for x in embeddings[i0]
               for y in embeddings[i1]):
            raise ValueError('no edges between logical variables %d and %d'
                             % (i0, i1))


def _clean_embedding(embeddings, adj, h, j):
    embedding_modified = list(embeddings)
    used = {i for i, x in enumerate(h) if x}

    for i, x in j.iteritems():
        if x:
            used.update(i)

    for i in range(0, len(embedding_modified)):
        if i not in used:
            embedding_modified[i] = embedding_modified[i][:1]

    new_embeddings = []
    interchain_vars = set()
    for u, v in j:
        for uemb in embedding_modified[u]:
            for vemb in embedding_modified[v]:
                if (uemb, vemb) in adj:
                    interchain_vars.add(uemb)
                    interchain_vars.add(vemb)

    for emb in embedding_modified:
        leaves = set()
        iev_nbrs = {}
        for u in emb:
            if u not in interchain_vars:
                nbrs = set(v for v in emb if u != v and (u, v) in adj)
                iev_nbrs[u] = nbrs
                if len(nbrs) == 1:
                    leaves.add(u)
        pruned = set()
        while leaves:
            leaf = leaves.pop()
            if not iev_nbrs[leaf]:
                continue
            pruned.add(leaf)
            nbr = iev_nbrs.pop(leaf).pop()
            nbr_nbrs = iev_nbrs.get(nbr)
            if nbr_nbrs is not None:
                nbr_nbrs.remove(leaf)
                if len(nbr_nbrs) == 1:
                    leaves.add(nbr)
        new_embeddings.append([v for v in emb if v not in pruned])

    return new_embeddings


def _smear_embedding(embeddings, adj, h, j, h_range, j_range):
    if h_range is None:
        h_range = (-1, 1)
    if h_range[0] >= 0 or h_range[1] <= 0:
        raise ValueError('h_range must include negative and postive values')
    if j_range is None:
        j_range = (-1, 1)
    if j_range[0] >= 0 or j_range[1] <= 0:
        raise ValueError('j_range must include negative and postive values')

    if not j:
        return embeddings

    j_mult = {(u, v): sum((x, y) in adj for x in embeddings[u]
                          for y in embeddings[v]) for u, v in j}
    j_scale = min(j_range[0 if v < 0 else 1] * j_mult[k] / v
                  for k, v in j.iteritems())

    h_scales = []
    for i, (v, emb) in enumerate(zip(h, embeddings)):
        if v < 0:
            h_scales.append((h_range[0] * len(emb) / v, i))
        elif v > 0:
            h_scales.append((h_range[1] * len(emb) / v, i))

    used = set(u for emb in embeddings for u in emb)
    adjv = {}
    for u, v in adj:
        if u < v:
            adjv.setdefault(u, set())
            adjv.setdefault(v, set())
            if u not in used:
                adjv[v].add(u)
            if v not in used:
                adjv[u].add(v)

    embd = {}
    for h_scale, i in sorted(h_scales):
        if h_scale >= j_scale:
            break
        target_size = j_scale * h[i] / h_range[0 if h[i] < 0 else 1]
        emb = list(embeddings[i])

        avail = None
        while len(emb) < target_size:
            if avail:
                u = avail.pop()
                used.add(u)
                emb.append(u)
            else:
                avail = set(v for u in emb for v in adjv[u] if v not in used)
                if not avail:
                    break
        embd[i] = emb

    return [embd.get(i, list(embeddings[i])) for i in xrange(len(embeddings))]


def embed_problem(h, j, embeddings, adj, clean=False, smear=False,
                  h_range=None, j_range=None):
    """
    embed_problem(h, j, embeddings, A, **params)

    Args:
        h: h value for ising problem

        j: j value for ising problem

        embeddings: an N-element list describing the embedding. Element k is a
        list of the output variables to which problem variable k is mapped.
        Elements (of the list) must be mutually disjoint.

        adj: a set describing the output adjacency structure. Output
        variables i and j are adjacent iff (i, j) is in A.

       clean: if clean embeddings
       h_range: h's range
       j_range: j's range
       smear: if smear embeddings

    Returns:
        h0, j0: Output problem.
            (If smear is set as True, values are scaled into the range
            [h_min, h_max] and [j_min, j_max])

        jc: Strong output variable couplings. jc[i][j] == -1 precisely when
        i < j (jc is upper triangular), (i, j) in A, and i and j
        correspond to the same problem variable.

        The reason that jc is returned in addition to j0 is that ising
        problems have no constraints. A problem variable may map to
        multiple output variable, so there is a real possibility that the
        optimal solution of the output problem has inconsistent values for
        those output variables. In this case, h0 and j0 can be scaled down
        relative to jc to produce a new Ising problem (h1, j1):

           h1 = s * h0;  j1 = s * j0 + jc;

        for some 0 < s < 1. Finding the right value for s is hard. It's
        easy to find a tiny value that will make every consistent solution
        better than any inconsistent solution, but this may raise the
        precision requirements beyond the capabilities of the quantum
        processor. Also, you may need only the best one or two consistent
        solutions, allowing a larger value of s. Bottom line: try s = 1 first
        and decrease until desired consistency is achieved."""

    num_vars = len(embeddings)
    if len(h) > num_vars:
        raise ValueError('h has more variables than embeddings')

    h = list(h) + [0] * (len(embeddings) - len(h))

    j_new = {}
    for (i0, i1), v in j.iteritems():
        if i0 == i1:
            raise ValueError('j contains diagonal elements')
        if i0 >= num_vars or i1 >= num_vars:
            raise ValueError('j has more variables than embeddings')
        k = tuple(sorted([i0, i1]))
        j_new[k] = j_new.get(k, 0) + v
    j_new = {k: v for k, v in j_new.iteritems() if v}

    adj_size = max(max(a) for a in adj) + 1 if adj else 0
    adj = (set(adj)
           | set((v, u) for u, v in adj)
           | set((u, u) for a in adj for u in a))

    _verify_embedding_1(embeddings, adj)
    _verify_embedding_2(embeddings, j_new, adj)

    if clean:
        embeddings = _clean_embedding(embeddings, adj, h, j_new)
    if smear:
        embeddings = _smear_embedding(embeddings, adj, h, j_new, h_range,
                                      j_range)

    h0 = [0.0] * adj_size
    j0 = {}
    jc = {}

    for emb, hi in zip(embeddings, h):
        emb_len = len(emb)
        for i, embv in enumerate(emb):
            h0[embv] = hi / emb_len
            for embw in emb[i + 1:]:
                if (embv, embw) in adj:
                    jc[tuple(sorted([embv, embw]))] = -1.0

    for k, v in j_new.iteritems():
        edges = []
        for emb1 in embeddings[k[0]]:
            for emb2 in embeddings[k[1]]:
                if (emb1, emb2) in adj:
                    edges.append(tuple(sorted([emb1, emb2])))
        ne = len(edges)
        for edge in edges:
            j0[edge] = j0.get(edge, 0) + v / ne

    return h0, j0, jc, embeddings


def _unembed_minenergy(sols, embeddings, h, j):
    usols = []
    for sol in sols:
        usol = []
        broken = []
        for i, emb in enumerate(embeddings):
            bits = set(sol[j] for j in emb)
            if len(bits) == 1:
                usol.append(bits.pop())
            else:
                usol.append(0)
                broken.append(i)

        energies = []
        for i in broken:
            e = h[i] if i < len(h) else 0
            e += sum(usol[k] * (j.get((i, k), 0) + j.get((k, i), 0))
                     for k in xrange(len(embeddings)))
            energies.append([-abs(e), e, i])
        heapq.heapify(energies)
        while energies:
            _, e, i = heapq.heappop(energies)
            usol[i] = -1 if e > 0 else 1
            for ee in energies:
                k = ee[2]
                ee[1] += usol[i] * (j.get((i, k), 0) + j.get((k, i), 0))
                ee[0] = -abs(ee[1])
            heapq.heapify(energies)

        usols.append(usol)

    return usols


def _unembed_vote(sols, embeddings):
    usols = []
    for sol in sols:
        usol = []
        for emb in embeddings:
            cp = 2 * sum(sol[i] == 1 for i in emb)
            if cp > len(emb):
                usol.append(1)
            elif cp < len(emb):
                usol.append(-1)
            else:
                usol.append(choice((1, -1)))
        usols.append(usol)
    return usols


def _unembed_discard(sols, embeddings):
    usols = []
    for sol in sols:
        usol = []
        for emb in embeddings:
            bits = set(sol[i] for i in emb)
            if len(bits) == 1:
                usol.append(bits.pop())
            else:
                break
        else:
            usols.append(usol)
    return usols


def _unembed_weighted_random(sols, embeddings):
    usols = []
    for sol in sols:
        usol = []
        for emb in embeddings:
            cp = sum(sol[i] == 1 for i in emb)
            b = 1 if random() < cp / len(emb) else -1
            usol.append(b)
        usols.append(usol)
    return usols


def unembed_answer(solutions, embeddings, broken_chains=None, h=None, j=None):
    """Convert embedded problem answers back to answers for original problem

    Args:
        solutions: Ising solution bits, i.e. a sequence of solutions,
            each of which is a sequence of +/-1 values.  Values other than
            +/-1 are permitted only in indices not part of the embedding.

        embeddings: same embedding as returned by embed_problem.

        broken_chains: strategy for repairing broken chains.  A broken chain
            is a set of variables representing a single logical variable
            whose values are not all equal.  Options:
            'minimize_energy' (default): a greedy heuristic that repairs
                broken chains one at a time, minimizing energy at each step.
            'vote': choose the most common value in the broken chain.  Ties
                are broken randomly.
            'discard': do not repair broken chains; returns solutions without
                any broken chains.
            'weighted_random': choose value 1 with probability equal to (#
                of 1's in the chain) / (chain size).

        h, j: original problem (before embedding).  Required when
            broken_chains is 'minimize_energy', ignored otherwise.

    Returns:
        solution bits for the original problem in the same format (list of
        lists) as solutions argument
    """

    if broken_chains is None or broken_chains == 'minimize_energy':
        return _unembed_minenergy(solutions, embeddings, h, j)
    elif broken_chains == 'vote':
        return _unembed_vote(solutions, embeddings)
    elif broken_chains == 'discard':
        return _unembed_discard(solutions, embeddings)
    elif broken_chains == 'weighted_random':
        return _unembed_weighted_random(solutions, embeddings)
    else:
        raise ValueError("Unknown broken_chains value: '%s'" % broken_chains)
