# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import pytest
from flexmock import flexmock
import sapilocal

import dwave_sapi2.local


def test_solver_names():
    solver_names = dwave_sapi2.local.local_connection.solver_names()
    assert set(solver_names) == {'c4-sw_sample', 'c4-sw_optimize',
                                 'ising-heuristic'}


def test_properties():
    sampler = dwave_sapi2.local.local_connection.get_solver('c4-sw_sample')
    assert set(sampler.properties.get('supported_problem_types')) == {
        'ising', 'qubo'}
    assert 'parameters' in sampler.properties

    opt = dwave_sapi2.local.local_connection.get_solver('c4-sw_optimize')
    assert set(opt.properties.get('supported_problem_types')) == {
        'ising', 'qubo'}
    assert 'parameters' in opt.properties

    isingh = dwave_sapi2.local.local_connection.get_solver('ising-heuristic')
    assert set(isingh.properties.get('supported_problem_types')) == {
        'ising', 'qubo'}
    assert 'parameters' in isingh.properties


def test_sample_defaults():
    solver = dwave_sapi2.local.local_connection.get_solver('c4-sw_sample')

    expected_args = {'answer_histogram': True, 'beta': 3.0, 'num_reads': 1,
                     'max_answers': 1, 'random_seed': None}
    expected_args.update(solver._config)
    (flexmock(sapilocal)
     .should_receive('orang_sample')
     .with_args('type', 'data', expected_args)
     .once())

    solver.solve('type', 'data', {})


def test_sample_params():
    solver = dwave_sapi2.local.local_connection.get_solver('c4-sw_sample')

    expected_args = {'answer_histogram': False, 'beta': 1.0, 'num_reads': 123,
                     'max_answers': 34, 'random_seed': 1234}
    expected_args.update(solver._config)
    (flexmock(sapilocal)
     .should_receive('orang_sample')
     .with_args('type', 'data', expected_args)
     .once())

    solver.solve('type', 'data', {'answer_mode': 'raw', 'beta': 1.0,
                                  'num_reads': 123, 'max_answers': 34,
                                  'random_seed': 1234})


def test_sample_default_max_answers():
    solver = dwave_sapi2.local.local_connection.get_solver('c4-sw_sample')

    expected_args = {'answer_histogram': False, 'beta': 1.0, 'num_reads': 123,
                     'max_answers': 123, 'random_seed': 9876}
    expected_args.update(solver._config)
    (flexmock(sapilocal)
     .should_receive('orang_sample')
     .with_args('type', 'data', expected_args)
     .once())

    solver.solve('type', 'data', {'answer_mode': 'raw', 'beta': 1.0,
                                  'num_reads': 123, 'random_seed': 9876})


def test_optimize_defaults():
    solver = dwave_sapi2.local.local_connection.get_solver('c4-sw_optimize')

    expected_args = {'answer_histogram': True, 'num_reads': 1,
                     'max_answers': 1}
    expected_args.update(solver._config)
    (flexmock(sapilocal)
     .should_receive('orang_optimize')
     .with_args('type', 'data', expected_args)
     .once())

    solver.solve('type', 'data', {})


def test_optimize_params():
    solver = dwave_sapi2.local.local_connection.get_solver('c4-sw_optimize')

    expected_args = {'answer_histogram': False, 'num_reads': 123,
                     'max_answers': 34}
    expected_args.update(solver._config)
    (flexmock(sapilocal)
     .should_receive('orang_optimize')
     .with_args('type', 'data', expected_args)
     .once())

    solver.solve('type', 'data',
                 {'answer_mode': 'raw', 'num_reads': 123, 'max_answers': 34})


def test_optimize_default_max_answers():
    solver = dwave_sapi2.local.local_connection.get_solver('c4-sw_optimize')

    expected_args = {'answer_histogram': False, 'num_reads': 123,
                     'max_answers': 123}
    expected_args.update(solver._config)
    (flexmock(sapilocal)
     .should_receive('orang_optimize')
     .with_args('type', 'data', expected_args)
     .once())

    solver.solve('type', 'data', {'answer_mode': 'raw', 'num_reads': 123})


def test_heuristic_defaults():
    solver = dwave_sapi2.local.local_connection.get_solver('ising-heuristic')

    (flexmock(sapilocal)
     .should_receive('orang_heuristic')
     .with_args('type', 'data', solver._default_params)
     .once())

    solver.solve('type', 'data', {})


def test_heuristic_params():
    solver = dwave_sapi2.local.local_connection.get_solver('ising-heuristic')

    expected_args = {'iteration_limit': 1000,
                     'max_bit_flip_prob': 0.25,
                     'max_local_complexity': 4,
                     'min_bit_flip_prob': 0.125,
                     'local_stuck_limit': 111,
                     'num_perturbed_copies': 3,
                     'num_variables': 45,
                     'time_limit_seconds': 2.0,
                     'random_seed': 777}
    (flexmock(sapilocal)
     .should_receive('orang_heuristic')
     .with_args('type', 'data', expected_args)
     .once())

    solver.solve('type', 'data', expected_args)
