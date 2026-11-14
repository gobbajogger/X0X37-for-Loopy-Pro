"""Lossless observation envelope; no J6-to-DnBOX semantic conversion."""
import base64
import binascii
from decimal import Decimal
import hashlib
import json

from schema import inspect_bank

FORMAT = 'dnbox-j6-observation-envelope/1'


def _json(value):
    return json.dumps(value, ensure_ascii=True, sort_keys=True,
                      separators=(',', ':'), allow_nan=False).encode('utf-8')


def map_bank(raw):
    """Index observed positions; raw bytes retain all unknown/lexical state.

    Number strings are decoded exact values, not original JSON tokens or units.
    No target IDs, defaults, normalization or unsupported-field deletion occur.
    """
    bank = inspect_bank(raw)
    parameters = []
    for pi, preset in enumerate(bank.data['presets']):
        for vi, parameter in enumerate(preset['parameterValues']):
            value = parameter['value']
            parameters.append({
                'pointer': f'/presets/{pi}/parameterValues/{vi}',
                'identifier': parameter['identifier'],
                'number': {'kind': 'decimal' if isinstance(value, Decimal) else 'integer',
                           'value': str(value)},
                'semantic_status': 'unresolved',
                'dnbox_target': None,
            })
    return _json({'format': FORMAT, 'raw_sha256': hashlib.sha256(raw).hexdigest(),
                  'raw_base64': base64.b64encode(raw).decode('ascii'),
                  'preset_count': bank.preset_count, 'parameters': parameters})


def restore_bank(envelope):
    """Validate integrity and rederive the index before returning exact bytes.

    This checks internal consistency, not source authenticity. Pin the original
    archive separately. Modified complete envelopes can represent different banks.
    """
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError('duplicate envelope key')
            result[key] = value
        return result

    if not isinstance(envelope, bytes):
        raise ValueError('envelope input must be bytes')
    try:
        data = json.loads(envelope.decode('utf-8'), object_pairs_hook=unique)
        if not isinstance(data, dict) or data.get('format') != FORMAT:
            raise ValueError('unknown envelope format')
        raw = base64.b64decode(data['raw_base64'], validate=True)
        if hashlib.sha256(raw).hexdigest() != data['raw_sha256']:
            raise ValueError('raw bank hash mismatch')
        # Canonical byte comparison distinguishes JSON true from integer 1 and
        # rejects invented targets, stale indexes, extra fields and type changes.
        if _json(data) != map_bank(raw):
            raise ValueError('observation index does not match raw bank')
        return raw
    except (UnicodeError, KeyError, TypeError, binascii.Error, RecursionError) as error:
        raise ValueError('invalid observation envelope') from error
