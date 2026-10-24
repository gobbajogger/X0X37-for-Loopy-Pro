#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Gate the code pipeline on a persisted stage thread.

Stages run in order: Audit, Implement, Test, Validate, Merge, Publish.
The caller starts a new top-level thread for a stage and passes that thread
id here. record_thread() stores the stage, thread id, timestamp, and status
in the ledger. Each stage needs its own thread id; an id already stored on
any transition is rejected before the ledger is saved. advance() reads the
ledger and moves the current stage only when that new id is already on disk.
note_helper() records a subagent on the stage that is already running.
A missing, null, subagent, or already-used id never changes the current stage.

The ledger is cloud-handoff/pipeline.json. Historical HANDOFF directories
stay evidence; this file is the record the pipeline consults.
"""
import argparse
import json
import os
from datetime import datetime
from pathlib import Path
import sys

SCHEMA = 1
STAGES = ('Audit', 'Implement', 'Test', 'Validate', 'Merge', 'Publish')
ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LEDGER = ROOT / 'cloud-handoff' / 'pipeline.json'


class HandoffError(ValueError):
    """The pipeline refused to record a handoff or to advance."""


def empty_ledger():
    return {
        'schema': SCHEMA,
        'stages': list(STAGES),
        'current': None,
        'transitions': [],
        'helpers': [],
    }


def _next(current):
    if current is None:
        return STAGES[0]
    return STAGES[STAGES.index(current) + 1] if current != STAGES[-1] else None


def _plain_id(value, message):
    if not isinstance(value, str) or value == '' or any(ch.isspace() for ch in value):
        raise HandoffError(message)
    return value


def _timestamp(value):
    if not isinstance(value, str) or value == '' or any(ch.isspace() for ch in value):
        raise HandoffError('timestamp required')
    text = value[:-1] + '+00:00' if value.endswith('Z') else value
    try:
        parsed = datetime.fromisoformat(text)
    except ValueError as error:
        raise HandoffError('timestamp must be ISO-8601 with a timezone') from error
    if parsed.tzinfo is None:
        raise HandoffError('timestamp must be ISO-8601 with a timezone')
    return value


def _stage(value):
    if value not in STAGES:
        raise HandoffError('unknown stage')
    return value


def save(path, state):
    """Replace the ledger only after its new bytes are flushed."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(f'.{path.name}.{os.getpid()}.tmp')
    data = (json.dumps(state, indent=2) + '\n').encode()
    try:
        descriptor = os.open(temporary, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o644)
        try:
            os.write(descriptor, data)
            os.fsync(descriptor)
        finally:
            os.close(descriptor)
        os.replace(temporary, path)
    except Exception:
        try:
            temporary.unlink()
        except OSError:
            pass
        raise
    directory = os.open(path.parent, os.O_RDONLY)
    try:
        os.fsync(directory)
    finally:
        os.close(directory)


def load(path):
    path = Path(path)
    if not path.is_file():
        raise HandoffError('no handoff ledger')
    try:
        state = json.loads(path.read_text(encoding='utf-8'))
    except (OSError, json.JSONDecodeError) as error:
        raise HandoffError('unsupported handoff ledger') from error
    if not isinstance(state, dict) or state.get('schema') != SCHEMA:
        raise HandoffError('unsupported handoff ledger')
    if state.get('stages') != list(STAGES):
        raise HandoffError('ledger stages do not match the pipeline')
    if 'current' not in state or 'transitions' not in state:
        raise HandoffError('unsupported handoff ledger')
    current = state['current']
    transitions = state['transitions']
    if current is not None and current not in STAGES:
        raise HandoffError('unsupported handoff ledger')
    if not isinstance(transitions, list) or any(not isinstance(item, dict) for item in transitions):
        raise HandoffError('unsupported handoff ledger')
    helpers = state.get('helpers', [])
    if not isinstance(helpers, list) or any(not isinstance(item, dict) for item in helpers):
        raise HandoffError('unsupported handoff ledger')
    state['helpers'] = helpers
    return state


def init(path):
    path = Path(path)
    if path.exists():
        raise HandoffError('handoff ledger already exists')
    state = empty_ledger()
    save(path, state)
    return state


def _helper_ids(state):
    return {item.get('subagent_id') for item in state['helpers']}


def record_thread(path, stage, thread_id, timestamp):
    """Store one new thread for the next stage. Do not change the current stage."""
    state = load(path)
    stage = _stage(stage)
    expected = _next(state['current'])
    if expected is None:
        raise HandoffError('pipeline is complete')
    if stage != expected:
        raise HandoffError('stage is out of order')
    if any(item.get('stage') == stage for item in state['transitions']):
        raise HandoffError('stage handoff already recorded')
    thread_id = _plain_id(thread_id, 'thread id required')
    if thread_id in _helper_ids(state):
        raise HandoffError('stage handoff requires a new thread, not a subagent')
    if any(item.get('thread_id') == thread_id for item in state['transitions']):
        raise HandoffError('each stage requires its own thread')
    state['transitions'].append({
        'stage': stage,
        'thread_id': thread_id,
        'timestamp': _timestamp(timestamp),
        'status': 'recorded',
        'kind': 'thread',
    })
    save(path, state)
    return state


def _recorded_thread(state, stage):
    found = [item for item in state['transitions'] if item.get('stage') == stage]
    if len(found) != 1:
        return None
    item = found[0]
    thread_id = item.get('thread_id')
    if item.get('kind') != 'thread' or item.get('status') != 'recorded':
        return None
    if not isinstance(thread_id, str) or thread_id == '' or any(ch.isspace() for ch in thread_id):
        return None
    if thread_id in _helper_ids(state):
        raise HandoffError('stage handoff requires a new thread, not a subagent')
    if any(other is not item and other.get('thread_id') == thread_id for other in state['transitions']):
        raise HandoffError('each stage requires its own thread')
    return item


def advance(path):
    """Move one stage forward only when its thread id is already in the ledger."""
    state = load(path)
    stage = _next(state['current'])
    if stage is None:
        raise HandoffError('pipeline is complete')
    entry = _recorded_thread(state, stage)
    if entry is None:
        raise HandoffError('pipeline does not advance until the thread id is recorded')
    state['current'] = stage
    entry['status'] = 'active'
    save(path, state)
    return state


def handoff(path, stage, thread_id, timestamp):
    """Persist the new thread, then advance by reading that persisted ledger."""
    record_thread(path, stage, thread_id, timestamp)
    return advance(path)


def note_helper(path, stage, subagent_id, timestamp):
    """Record in-stage help. This does not hand the stage to the subagent."""
    state = load(path)
    stage = _stage(stage)
    if state['current'] != stage:
        raise HandoffError('subagents only help inside a stage that is already running')
    subagent_id = _plain_id(subagent_id, 'subagent id required')
    if any(item.get('thread_id') == subagent_id for item in state['transitions']):
        raise HandoffError('stage handoff requires a new thread, not a subagent')
    if any(item.get('stage') == stage and item.get('subagent_id') == subagent_id for item in state['helpers']):
        raise HandoffError('subagent already recorded for this stage')
    state['helpers'].append({
        'stage': stage,
        'subagent_id': subagent_id,
        'timestamp': _timestamp(timestamp),
        'status': 'in-stage',
    })
    save(path, state)
    return state


def _emit(state):
    json.dump(state, sys.stdout, indent=2)
    print()


def _command(commands, name, help_text):
    command = commands.add_parser(name, help=help_text)
    command.add_argument('--ledger', type=Path, default=argparse.SUPPRESS)
    return command


def main(argv=None):
    parser = argparse.ArgumentParser(description='Persist a stage thread before the pipeline advances.')
    parser.add_argument('--ledger', type=Path, default=DEFAULT_LEDGER)
    commands = parser.add_subparsers(dest='command', required=True)

    _command(commands, 'init', 'create the handoff ledger')
    _command(commands, 'show', 'print the handoff ledger')
    _command(commands, 'advance', 'enter the next stage when its thread id is stored')

    recorded = _command(commands, 'record', 'store the next stage thread without advancing')
    recorded.add_argument('--stage', required=True)
    recorded.add_argument('--thread-id', required=True)
    recorded.add_argument('--timestamp', required=True)

    delivered = _command(commands, 'handoff', 'store the next stage thread, then advance')
    delivered.add_argument('--stage', required=True)
    delivered.add_argument('--thread-id', required=True)
    delivered.add_argument('--timestamp', required=True)

    helper = _command(commands, 'helper', 'note a subagent helping the running stage')
    helper.add_argument('--stage', required=True)
    helper.add_argument('--subagent-id', required=True)
    helper.add_argument('--timestamp', required=True)

    args = parser.parse_args(argv)
    path = args.ledger
    if args.command == 'init':
        state = init(path)
    elif args.command == 'show':
        state = load(path)
    elif args.command == 'advance':
        state = advance(path)
    elif args.command == 'record':
        state = record_thread(path, args.stage, args.thread_id, args.timestamp)
    elif args.command == 'handoff':
        state = handoff(path, args.stage, args.thread_id, args.timestamp)
    else:
        state = note_helper(path, args.stage, args.subagent_id, args.timestamp)
    _emit(state)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except HandoffError as error:
        print('Stage handoff: ' + str(error), file=sys.stderr)
        sys.exit(1)
