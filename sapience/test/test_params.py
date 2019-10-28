# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import pytest
import sapience


def test_orang_config_empty():
    config = sapience.orang_config([], [])
    assert len(config) == 4
    assert config['num_vars'] == sapience.C4_NUMQUBITS
    assert not config['active_vars']
    assert not config['active_var_pairs']
    assert not config['var_order']


def test_orang_config_full():
    config = sapience.orang_config(list(range(sapience.C4_NUMQUBITS)),
                                   sapience.C4_COUPLERS)
    assert len(config) == 4
    assert config['num_vars'] == sapience.C4_NUMQUBITS
    assert set(config['active_vars']) == set(range(sapience.C4_NUMQUBITS))
    assert set(config['active_var_pairs']) == set(sapience.C4_COUPLERS)
    assert list(config['var_order']) == list(sapience.C4_VARORDER)


def test_orang_config_some():
    config = sapience.orang_config(list(range(0, sapience.C4_NUMQUBITS, 2)),
                                   [(0, 4), (1, 5)])
    assert len(config) == 4
    assert config['num_vars'] == sapience.C4_NUMQUBITS
    assert set(config['active_vars']) == set(
        range(0, sapience.C4_NUMQUBITS, 2))
    assert set(config['active_var_pairs']) == {(0, 4), (1, 5)}
    assert list(config['var_order']) == [i for i in sapience.C4_VARORDER if
                                         i % 2 == 0]


def test_opt_defaults():
    params = sapience.optimization_params({}, [], [])
    assert len(params) == 7
    assert params['num_vars'] == sapience.C4_NUMQUBITS
    assert not params['active_vars']
    assert not params['active_var_pairs']
    assert not params['var_order']
    assert params['num_reads'] == 1
    assert params['answer_histogram'] == True
    assert params['max_answers'] == 1


@pytest.mark.parametrize("num_reads,max_answers,answer_mode",
                         [(10, None, 'histogram'),
                          (None, 20, 'raw'),
                          ("4", "2", None)])
def test_opt_params_good(num_reads, max_answers, answer_mode):
    user_params = {}
    if num_reads is not None:
        user_params['num_reads'] = num_reads
    if max_answers is not None:
        user_params['max_answers'] = max_answers
    if answer_mode is not None:
        user_params['answer_mode'] = answer_mode
    params = sapience.optimization_params(user_params, [], [])
    assert len(params) == 7
    assert params['num_vars'] == sapience.C4_NUMQUBITS
    assert not params['active_vars']
    assert not params['active_var_pairs']
    assert not params['var_order']
    assert params['num_reads'] == int(
        num_reads) if num_reads is not None else 1
    assert params['answer_histogram'] == (answer_mode != 'raw')
    assert params['max_answers'] == int(
        max_answers) if max_answers is not None else params['num_reads']


@pytest.mark.parametrize("num_reads,max_answers,answer_mode",
                         [('nope', 1, 'histogram'),
                          (-1, 1, 'histogram'),
                          (sapience.MAX_NUM_READS + 1, 1, 'histogram'),
                          (1, 'nope', 'histogram'),
                          (1, -1, 'histogram'),
                          (1, 1, 'blah')])
def test_opt_params_bad(num_reads, max_answers, answer_mode):
    with pytest.raises(sapience.UserError):
        sapience.optimization_params({'num_reads': num_reads,
                                      'max_answers': max_answers,
                                      'answer_mode': answer_mode}, [], [])


def test_opt_params_badname():
    with pytest.raises(sapience.UserError):
        sapience.optimization_params({'_nope': 123}, [], [])


def test_samp_defaults():
    params = sapience.sampling_params({}, list(range(sapience.C4_NUMQUBITS)),
                                                sapience.C4_COUPLERS)
    assert len(params) == 8
    assert params['num_vars'] == sapience.C4_NUMQUBITS
    assert set(params['active_vars']) == set(range(sapience.C4_NUMQUBITS))
    assert set(params['active_var_pairs']) == set(sapience.C4_COUPLERS)
    assert list(params['var_order']) == list(sapience.C4_VARORDER)
    assert params['num_reads'] == 1
    assert params['answer_histogram'] == True
    assert params['max_answers'] == 1
    assert params['beta'] == 3.0


@pytest.mark.parametrize("num_reads,max_answers,answer_mode,beta",
                         [(10, None, 'histogram', None),
                          (None, 20, 'raw', 1.0),
                          ("4", "2", None, '1.5')])
def test_samp_params_good(num_reads, max_answers, answer_mode, beta):
    user_params = {}
    if num_reads is not None:
        user_params['num_reads'] = num_reads
    if max_answers is not None:
        user_params['max_answers'] = max_answers
    if answer_mode is not None:
        user_params['answer_mode'] = answer_mode
    if beta is not None:
        user_params['beta'] = beta
    params = sapience.sampling_params(user_params, [], [])
    assert len(params) == 8
    assert params['num_vars'] == sapience.C4_NUMQUBITS
    assert not params['active_vars']
    assert not params['active_var_pairs']
    assert not params['var_order']
    assert params['num_reads'] == int(
        num_reads) if num_reads is not None else 1
    assert params['answer_histogram'] == (answer_mode != 'raw')
    assert params['max_answers'] == int(
        max_answers) if max_answers is not None else params['num_reads']
    assert params['beta'] == float(beta) if beta is not None else 3.0


@pytest.mark.parametrize("num_reads,max_answers,answer_mode,beta",
                         [('nope', 1, 'histogram', 3.0),
                          (-1, 1, 'histogram', 3.0),
                          (sapience.MAX_NUM_READS + 1, 1, 'histogram', 3.0),
                          (1, 'nope', 'histogram', 3.0),
                          (1, -1, 'histogram', 3.0),
                          (1, 1, 'blah', 3.0),
                          (1, 1, 'histogram', -1.0),
                          (1, 1, 'histogram', float('inf')),
                          (1, 1, 'histogram', 'nope')])
def test_samp_params_bad(num_reads, max_answers, answer_mode, beta):
    with pytest.raises(sapience.UserError):
        sapience.sampling_params({'num_reads': num_reads,
                                  'max_answers': max_answers,
                                  'answer_mode': answer_mode,
                                  'beta': beta}, [], [])


def test_samp_params_badname():
    with pytest.raises(sapience.UserError):
        sapience.optimization_params({'_nope': 123}, [], [])


def test_opt_ignoreparams():
    sapience.optimization_params(dict.fromkeys(sapience.IGNORE_PARAMS), [], [])


def test_samp_ignoreparams():
    sapience.sampling_params(dict.fromkeys(sapience.IGNORE_PARAMS), [], [])
