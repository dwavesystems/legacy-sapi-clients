*********************************************************************
Migrating to Ocean from the Legacy SAPI Clients
*********************************************************************

Long-term users of D-Wave systems have used a set of SAPI client
libraries (the *legacy SAPI clients*) to interact with D-Wave
solvers. Binaries for these clients are available for download
here under a standard EULA license.

**Note:** You can build the clients yourself using the ``cmake`` command from the ``cmake-modules``
folder in this repo. See the ``cmake`` `documentation <https://cmake.org/runningcmake/>`_ for more information.

These packages, predecessors to D-Wave's open-source `Ocean SDK <https://github.com/dwavesystems/dwave-ocean-sdk>`_,
are in the process of being **deprecated**. After December 31, 2019,
D-Wave support for these clients will be discontinued. For this reason, we
recommend that you transition your code to Ocean as soon as feasible.
The guide below maps the legacy Python functions to their Ocean equivalents.

If you use:

* **Python:** Migration to Ocean is straightforward; see the mapping of functions provided below.
  Be aware that the legacy Python client supports Python 2 only, so you will need to move to Ocean for Python 3.
* **MATLAB:** If you do not wish to migrate your code to Python, you can build a MATLAB integration with the Ocean tools.
* **C:** We strongly recommend that you switch to Python to get the latest features available in Ocean.

For more information on using the Ocean tools, see the `Ocean documentation <https://docs.ocean.dwavesys.com>`_. Specifically,
if you have not yet installed the Ocean tools, see the `Getting Started guide <https://docs.ocean.dwavesys.com/en/latest/getting_started.html#gs>`_.
You may also want to review the `list of Ocean tools <https://docs.ocean.dwavesys.com/en/latest/projects.html#projects>`_. 

.. contents:: Contents
  :depth: 2



Solvers
===============================

In most cases, it is not necessary to create a remote connection manually in Ocean.
Instead, a solver can be created simply:

.. code:: python

  from dwave.system.samplers import DWaveSampler
  sampler = DWaveSampler(endpoint = url, token = token, proxy = proxy)

Make a Remote Connection
--------------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.remote import RemoteConnection
  remote_connection = RemoteConnection(url, token)
  remote_connection = RemoteConnection(url, token, proxy_url)
  solver_names = remote_connection.solver_names()
  solver = remote_connection.get_solver("name")


Ocean Tools
.........................

.. code:: python

  from dwave.cloud import Client
  client = Client (endpoint = url, token = token)
  client = Client (endpoint = url, token = token, proxy = proxy)
  solver_names = client.get_solvers()
  solver = client.get_solver ("name")

Class Reference:

`class dwave.cloud.client.Client(endpoint=None, token=None, solver=None, proxy=None, permissive_ssl=False, request_timeout=60, polling_timeout=None, connection_close=False, **kwargs)
<https://docs.ocean.dwavesys.com/projects/cloud-client/en/latest/reference/resources.html?highlight=proxy#dwave.cloud.client.Client>`_


Make a Local Connection
-----------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.local import local_connection
  solver_names = local_connection.solver_names()
  solver = local_connection.get_solver("name")

Ocean Tools
.........................

.. code:: python

  from dwave.cloud import Client
  solver_names = client.get_solvers()
  solver = client.get_solver ("name")


Class Reference:

`class DWaveSampler(**config) <https://docs.ocean.dwavesys.com/projects/system/en/latest/reference/samplers.html#dwavesampler>`_

`class dwave.cloud.solver.BaseSolver(client, data) <https://docs.ocean.dwavesys.com/projects/cloud-client/en/latest/reference/solver.html?highlight=solver#dwave.cloud.solver.BaseSolver>`_


Retrieve Solvers
--------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.remote import RemoteConnection
  url = 'system-url'
  token = 'your-api-token'
  conn = RemoteConnection(url, token)
  print conn.solver_names()


Ocean Tools
.........................

.. code:: python

  from dwave.cloud import Client
  url = 'system-url'
  token = 'your-api-token'
  client = Client(endpoint=url, token=token)
  print(client.get_solvers())

Class Reference:

`class dwave.cloud.client.Client(endpoint=None, token=None, solver=None, proxy=None, permissive_ssl=False, request_timeout=60, polling_timeout=None, connection_close=False, **kwargs)
<https://docs.ocean.dwavesys.com/projects/cloud-client/en/latest/reference/resources.html?highlight=proxy#dwave.cloud.client.Client>`_



Problems
======================

Solve an Ising Problem (Synchronous)
---------------------------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.core import solve_ising
  answer = solve_ising(solver, h, J)
  answer = solve_ising(solver, h, J, param_name=value, ...)


Ocean Tools
.........................

.. code:: python

  sampler = DWaveSampler()
  response = sampler.sample_ising(h, J)
  response = sampler.sample_ising(h, J, param_name=value, …)

Class Reference:

`class DWaveSampler(**config) <https://docs.ocean.dwavesys.com/projects/system/en/latest/reference/samplers.html#dwavesampler>`_


Solve a QUBO Problem (Synchronous)
----------------------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.core import solve_qubo
  answer = solve_qubo(solver, Q)
  answer = solve_qubo(solver, Q, param_name=value, ...)

Ocean Tools
.........................

.. code:: python

  sampler = DWaveSampler()
  response = sampler.sample_qubo (h, J)
  response = sampler.sample_qubo (h, J, param_name=value, …)

Class Reference:

`class DWaveSampler(**config) <https://docs.ocean.dwavesys.com/projects/system/en/latest/reference/samplers.html#dwavesampler>`_

Solve an Ising Problem (Asynchronous)
----------------------------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.core import async_solve_ising
  submitted_problem = async_solve_ising(solver, h, J)
  submitted_problem = async_solve_ising(solver, h, J, param_name=value, ...)


Ocean Tools
.........................

.. code:: python

  solver = client.get_solver ("name")
  future = solver.sample_ising(h, J)
  future = solver.sample_ising(h, J, param_name=value, …)
  class dwave.cloud.computation.Future(solver, id_, return_matrix=False)


Class Reference:

`class dwave.cloud.computation.Future(solver, id_, return_matrix=False)
<https://docs.ocean.dwavesys.com/projects/cloud-client/en/latest/reference/computation.html#dwave.cloud.computation.Future>`_


Solve a QUBO Problem (Asynchronous)
----------------------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.core import async_solve_qubo, await_completion
  submitted_problem = async_solve_qubo(solver, Q)
  submitted_problem = async_solve_qubo(solver, Q, param_name=value, ...)


Ocean Tools
.........................

.. code:: python

  solver = client.get_solver ("name")
  future = solver.sample_qubo(h, J)
  future = solver.sample_qubo (h, J, param_name=value, …)

Class Reference:

`class dwave.cloud.computation.Future(solver, id_, return_matrix=False)
<https://docs.ocean.dwavesys.com/projects/cloud-client/en/latest/reference/computation.html#dwave.cloud.computation.Future>`_


Await Completion
-------------------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.core import await_completion
  done = await_completion(submitted_problems, min_done, timeout)


Ocean Tools
.........................

.. code:: python

  solver = client.get_solver ("name")
  future = solver.sample_ising(h, J)
  future.wait(timeout = timeout)


Class Reference:

`class dwave.cloud.computation.Future(solver, id_, return_matrix=False)
<https://docs.ocean.dwavesys.com/projects/cloud-client/en/latest/reference/computation.html#dwave.cloud.computation.Future>`_

Embedding
=================

Find Embedding
--------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.embedding import find_embedding
  embeddings = find_embedding(S, A)
  embeddings = find_embedding(S, A, param_name=value, ...)

Ocean Tools
.........................

.. code:: python

  from minorminer import find_embedding
  emb = find_embedding(S, A)


Function Reference:

`find_embedding(S, T, **params)
<https://docs.ocean.dwavesys.com/projects/system/en/latest/reference/generated/minorminer.find_embedding.html?highlight=find_embedding#minorminer.find_embedding>`_

Embed Problem
------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.embedding import embed_problem
  [h0, j0, jc, embeddings] = embed_problem(h, j, embeddings, adj, clean, smear, h_range, j_range)

Ocean Tools
.........................

.. code:: python

  from dwave.embedding import embed_ising
  th, tJ = embed_ising(h, J, embedding, target)


Function Reference:

`embed_ising(source_h, source_J, embedding, target_adjacency, chain_strength=1.0)
<https://docs.ocean.dwavesys.com/projects/system/en/latest/reference/generated/dwave.embedding.embed_ising.html#dwave.embedding.embed_ising>`_

`embed_qubo(source_Q, embedding, target_adjacency, chain_strength=1.0)
<https://docs.ocean.dwavesys.com/projects/system/en/latest/reference/generated/dwave.embedding.embed_qubo.html#dwave.embedding.embed_qubo>`_

`embed_bqm(source_bqm, embedding, target_adjacency, chain_strength=1.0, smear_vartype=None)
<https://docs.ocean.dwavesys.com/projects/system/en/latest/reference/generated/dwave.embedding.embed_bqm.html#dwave.embedding.embed_bqm>`_

Unembed Answer
--------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.embedding import unembed_answer
  result = unembed_answer(solutions, embeddings, broken_chains=None, h=None, j=None)


Ocean Tools
.........................

.. code:: python

  from dwave.embedding import unembed_sampleset
  samples = unembed_sampleset(embedded, embedding, bqm)


This technique uses the ``bqm`` object, an abstraction of the Ising and QUBO forms.

Function Reference:

`unembed_sampleset(target_sampleset, embedding, source_bqm, chain_break_method=None, chain_break_fraction=False)
<https://docs.ocean.dwavesys.com/projects/system/en/latest/reference/generated/dwave.embedding.unembed_sampleset.html#dwave.embedding.unembed_sampleset>`_

Utilities
======================

Convert Ising to QUBO
----------------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.util import ising_to_qubo
  (Q, qubo_offset) = ising_to_qubo(h, J)


Ocean Tools
.........................

.. code:: python

  from dimod import ising_to_qubo
  (Q, qubo_offset) = ising_to_qubo(h, J)


This technique uses the ``bqm`` object, an abstraction of the Ising and QUBO forms.
Using this technique, it is not necessary to convert between Ising and QUBO formats
except to output the results; for example:

.. code:: python

  from dimod import BinaryQuadraticModel as BQM
  bqm = BQM.from_qubo(h, j, offset)
  qubo = bqm.to_ising()



Function Reference:

`ising_to_qubo(h, J, offset=0.0)
<https://docs.ocean.dwavesys.com/projects/dimod/en/0.7.6/reference/utilities.html?highlight=ising_to_qubo#dimod.utilities.ising_to_qubo>`_

Convert QUBO to Ising
----------------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.util import qubo_to_ising
  (h, J, ising_offset) = qubo_to_ising(Q)


Ocean Tools
.........................

.. code:: python

  from dimod import qubo_to_ising
  (h, J, ising_offset) = qubo_to_ising(Q)

Best practice for Ocean tools is to use the ``bqm`` object, which is an abstraction
of QUBO and Ising forms. Using this technique, it is not necessary to convert between
Ising and QUBO formats except to output the results; for example:

.. code:: python

  from dimod import BinaryQuadraticModel as BQM
  bqm = BQM.from_qubo(h, j, offset)
  qubo = bqm.to_ising()


Function Reference:

`qubo_to_ising(Q, offset=0.0)
<https://docs.ocean.dwavesys.com/projects/dimod/en/0.7.6/reference/utilities.html?highlight=ising_to_qubo#dimod.utilities.qubo_to_ising>`_

Get Chimera Adjacency
--------------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.util import get_chimera_adjacency
  A = get_chimera_adjacency(m, n, t)


Ocean Tools
.........................

.. code:: python

  from dwave_networkx import chimera_graph
  G = chimera_graph(m, n, t)
  dict(G.adjacency())
  chimera_graph(m, n=None, t=None, create_using=None, node_list=None, edge_list=None, data=True, coordinates=False)


Function Reference:

`chimera_graph(m, n=None, t=None, create_using=None, node_list=None, edge_list=None, data=True, coordinates=False)
<https://docs.ocean.dwavesys.com/projects/dwave-networkx/en/latest/reference/generated/dwave_networkx.generators.chimera_graph.html?highlight=chimera_graph#dwave_networkx.generators.chimera_graph>`_

Get Hardware Adjacency
------------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.util import get_hardware_adjacency
  A = get_hardware_adjacency(solver)

Ocean Tools
.........................

.. code:: python

  from dwave.system.samplers import DWaveSampler
  sampler = DWaveSampler(endpoint = url, token = token, proxy = proxy)
  A = sampler.adjacency

Class Reference:

`class DWaveSampler(**config)
<https://docs.ocean.dwavesys.com/projects/system/en/latest/reference/samplers.html#dwavesampler>`_

`class dwave.cloud.client.Client(endpoint=None, token=None, solver=None, proxy=None, permissive_ssl=False, request_timeout=60, polling_timeout=None, connection_close=False, **kwargs) <https://docs.ocean.dwavesys.com/projects/cloud-client/en/latest/reference/resources.html?highlight=proxy#dwave.cloud.client.Client>`_



Convert Linear Index to Chimera
-----------------------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.util import linear_index_to_chimera
  ind = linear_index_to_chimera(linear_index, m, n, t)

Ocean Tools
.........................

.. code:: python

  from dwave_networkx import linear_to_chimera
  ind = linear_to_chimera(r, m, n=None, t=None)


Convert Chimera to Linear Index
----------------------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.util import chimera_to_linear_index
  ind = chimera_to_linear_index(i, j, u, k, m, n, t)

Ocean Tools
.........................

.. code:: python

  from dwave_networkx import chimera_to_linear
  ind = chimera_to_linear(i, j, u, k, m, n, t)

Reduce Degree
-------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.util import reduce_degree
  (new_terms, vars_rep) = reduce_degree(terms)


Ocean Tools
.........................

.. code:: python

  from dimod import make_quadratic
  poly = {(0,): -1, (1,): 1, (2,): 1.5, (0, 1): -1, (0, 1, 2): -2}
  bqm = make_quadratic(poly, 5.0, dimod.SPIN)


Function Reference:

`make_quadratic(poly, strength, vartype=None, bqm=None)
<https://docs.ocean.dwavesys.com/projects/dimod/en/0.7.6/reference/generated/dimod.make_quadratic.html#dimod-make-quadratic>`_

Make Quadratic
-------------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.util import make_quadratic
  (Q, new_terms, vars_rep) = make_quadratic(f, penalty_weight=None)

Ocean Tools
.........................

.. code:: python

  from dimod import make_quadratic
  poly = {(0,): -1, (1,): 1, (2,): 1.5, (0, 1): -1, (0, 1, 2): -2}
  bqm = make_quadratic(poly, 5.0, dimod.SPIN)


Function Reference:

`make_quadratic(poly, strength, vartype=None, bqm=None)
<https://docs.ocean.dwavesys.com/projects/dimod/en/0.7.6/reference/generated/dimod.make_quadratic.html#dimod-make-quadratic>`_

Fix Variables
-----------------

Legacy Python Client
.........................

.. code:: python

  from dwave_sapi2.fix_variables import fix_variables
  result = fix_variables(q, method="optimized")

Ocean Tools
.........................

.. code:: python

  from dimod import fix_variables, BinaryQuadraticModel as BQM
  import dimod
  bqm = BQM.from_ising(h, J, offset)
  fixed_dict = dimod.fix_variables(bqm)

Class Reference:

`class BinaryQuadraticModel(linear, quadratic, offset, vartype, **kwargs) <https://docs.ocean.dwavesys.com/projects/dimod/en/latest/reference/bqm/binary_quadratic_model.html#dimod.BinaryQuadraticModel>`_

Function Reference:

`fix_variables(bqm, sampling_mode=True) <https://docs.ocean.dwavesys.com/projects/dimod/en/latest/reference/bqm/generated/dimod.roof_duality.fix_variables.html?highlight=fix_variables#dimod.roof_duality.fix_variables>`_

QSage
==========

Currently, there is no equivalent QSage functionality in Ocean tool suite.
This `Leap Community post <https://support.dwavesys.com/hc/en-us/community/posts/360026065734-QSage>`_ discusses the topic.

qbsolv
=============

The ``qbsolv`` utility has been replaced with the ``dwave-hybrid`` framework in Ocean
(it is possible to build a ``qbsolv`` replica with Ocean). Read more about `D-Wave Hybrid <https://docs.ocean.dwavesys.com/projects/hybrid/en/stable/>`_.
