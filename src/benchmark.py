import os
import sys
import shutil
import numpy
import parsable
import loom.config
import loom.runner
import loom.format
import loom.cFormat
import loom.test.util
from loom.util import parallel_map
from distributions.io.stream import json_load, protobuf_stream_load
parsable = parsable.Parsable()

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, 'data')
DATASETS = os.path.join(DATA, 'datasets')
RESULTS = os.path.join(DATA, 'results')
MODEL = os.path.join(DATASETS, '{}/model.pb.gz')
GROUPS = os.path.join(DATASETS, '{}/groups')
ASSIGN = os.path.join(DATASETS, '{}/assign.pbs.gz')
ROWS = os.path.join(DATASETS, '{}/rows.pbs.gz')


def mkdir_p(dirname):
    'like mkdir -p'
    if not os.path.exists(dirname):
        os.makedirs(dirname)


def list_options_and_exit(*required):
    print 'try one of:'
    for name in loom.test.util.list_datasets():
        if all(os.path.exists(patt.format(name)) for patt in required):
            print '  {}'.format(name)
    sys.exit(1)


parsable.command(loom.runner.profilers)


@parsable.command
def load(name=None, debug=False):
    '''
    Import a datasets, list available datasets, or load 'all' datasets.
    '''
    if name is None:
        list_options_and_exit()

    names = [name]

    if names == ['all']:
        names = [
            n
            for n in loom.test.util.list_datasets()
            if not os.path.exists(ROWS.format(n))
        ]

    args = [(n, debug) for n in names]
    parallel_map(_load, args)


def _load((name, debug)):
    print 'loading', name
    data_path = os.path.join(DATASETS, name)
    mkdir_p(data_path)
    model = MODEL.format(name)
    groups = GROUPS.format(name)
    assign = ASSIGN.format(name)
    rows = ROWS.format(name)

    dataset = loom.test.util.get_dataset(name)
    meta = dataset['meta']
    data = dataset['data']
    mask = dataset['mask']
    latent = dataset['latent']
    tardis_conf = dataset['tardis_conf']

    loom.format.import_latent(
        meta_in=meta,
        latent_in=latent,
        tardis_conf_in=tardis_conf,
        model_out=model,
        groups_out=groups,
        assign_out=assign)
    loom.format.import_data(
        meta_in=meta,
        data_in=data,
        mask_in=mask,
        rows_out=rows,
        validate=debug)
    loom.runner.shuffle(rows_in=rows, rows_out=rows)

    meta = json_load(meta)
    object_count = len(meta['object_pos'])
    feature_count = len(meta['feature_pos'])
    print '{}: {} rows x {} cols'.format(name, object_count, feature_count)


@parsable.command
def info(name=None, debug=False):
    '''
    Get information about a dataset, or list available datasets.
    '''
    if name is None:
        list_options_and_exit(ROWS)

    if debug:
        pos = 0
        dumped = 'None'
        sizes = []
        try:
            rows = loom.cFormat.protobuf_stream_load(ROWS.format(name))
            for pos, row in enumerate(rows):
                dumped = row.dump()
                sizes.append(row.ByteSize())
        except:
            print 'error after row {} with data:\n{}'.format(pos, dumped)
            raise
    else:
        rows = loom.cFormat.protobuf_stream_load(ROWS.format(name))
        sizes = [row.ByteSize() for row in rows]

    print 'row count:\t{}'.format(len(sizes))
    print 'min bytes:\t{}'.format(min(sizes))
    print 'mean bytes:\t{}'.format(numpy.mean(sizes))
    print 'max bytes:\t{}'.format(max(sizes))


@parsable.command
def shuffle(name=None, debug=False, profile='time'):
    '''
    Shuffle dataset for inference.
    '''
    if name is None:
        list_options_and_exit(ROWS)

    rows_in = ROWS.format(name)
    assert os.path.exists(rows_in), 'First load dataset'

    results_path = os.path.join(RESULTS, name)
    mkdir_p(results_path)
    rows_out = os.path.join(results_path, 'rows.pbs.gz')

    loom.runner.shuffle(
        rows_in=rows_in,
        rows_out=rows_out,
        debug=debug,
        profile=profile)
    assert os.path.exists(rows_out)


@parsable.command
def infer(
        name=None,
        extra_passes=0.0,
        debug=False,
        profile='time'):
    '''
    Run inference on a dataset, or list available datasets.
    '''
    if name is None:
        list_options_and_exit(ROWS)

    model = MODEL.format(name)
    rows = ROWS.format(name)
    assert os.path.exists(model), 'First load dataset'
    assert os.path.exists(rows), 'First load dataset'

    if extra_passes > 0:
        print 'Learning structure from scratch'
        groups_in = None
    else:
        print 'Priming structure with known groups'
        groups_in = GROUPS.format(name)
        assert os.path.exists(groups_in), 'First load dataset'

    results_path = os.path.join(RESULTS, name)
    mkdir_p(results_path)
    groups_out = os.path.join(results_path, 'groups')
    mkdir_p(groups_out)

    config = {'schedule': {'extra_passes': extra_passes}}
    config_in = os.path.join(results_path, 'config.pb.gz')
    loom.config.config_dump(config, config_in)

    loom.runner.infer(
        config_in=config_in,
        rows_in=rows,
        model_in=model,
        groups_in=groups_in,
        groups_out=groups_out,
        debug=debug,
        profile=profile)

    assert os.listdir(groups_out), 'no groups were written'
    group_counts = []
    for f in os.listdir(groups_out):
        group_count = 0
        for _ in protobuf_stream_load(os.path.join(groups_out, f)):
            group_count += 1
        group_counts.append(group_count)
    print 'group_counts: {}'.format(' '.join(map(str, group_counts)))


@parsable.command
def clean():
    '''
    Clean out data and results.
    '''
    for path in [DATASETS, RESULTS]:
        if os.path.exists(path):
            shutil.rmtree(path)


if __name__ == '__main__':
    parsable.dispatch()
