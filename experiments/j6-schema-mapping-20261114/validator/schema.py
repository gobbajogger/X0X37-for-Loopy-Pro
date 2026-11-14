"""Read-only observed-shape validator. Not an official J6 schema or converter."""
from dataclasses import dataclass
from decimal import Decimal, DecimalException
import json
from collections import Counter

@dataclass(frozen=True)
class BankInspection:
    raw: bytes
    data: dict
    coverage: dict
    preset_count: int

def _object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('duplicate JSON object key: ' + key)
        result[key] = value
    return result

def _constant(value):
    raise ValueError('non-JSON numeric constant: ' + value)

def inspect_bank(raw):
    """Preserve original bytes and all decoded fields; inspect only observed core shape.

    Decimal retains exact decimal numerical values, including out-of-range values.
    Original raw bytes retain lexical formatting. No serialization or mutation occurs.
    Acceptance means structurally inspectable, not that J6 will import the bank.
    """
    if not isinstance(raw, bytes):
        raise ValueError('bank input must be bytes')
    if len(raw) > 8 * 1024 * 1024:
        raise ValueError('bank exceeds bounded inspection size')
    try:
        data = json.loads(raw.decode('utf-8'), parse_float=Decimal,
                          parse_constant=_constant, object_pairs_hook=_object)
    except DecimalException as error:
        raise ValueError('JSON decimal exceeds supported representation') from error
    except (UnicodeError, json.JSONDecodeError, RecursionError) as error:
        raise ValueError('invalid UTF-8 JSON bank') from error
    if not isinstance(data, dict) or not isinstance(data.get('presets'), list):
        raise ValueError('bank must contain presets array')
    coverage = Counter()
    for preset in data['presets']:
        if not isinstance(preset, dict) or not isinstance(preset.get('parameterValues'), list):
            raise ValueError('preset must contain parameterValues array')
        seen = set()
        for parameter in preset['parameterValues']:
            if not isinstance(parameter, dict):
                raise ValueError('parameter must be an object')
            identifier, value = parameter.get('identifier'), parameter.get('value')
            if not isinstance(identifier, str) or not identifier or identifier in seen:
                raise ValueError('parameter identifier missing, empty or duplicated')
            if isinstance(value, bool) or not isinstance(value, (int, Decimal)):
                raise ValueError('parameter value must be a finite JSON number')
            if isinstance(value, Decimal) and not value.is_finite():
                raise ValueError('nonfinite parameter value')
            seen.add(identifier)
            coverage[identifier] += 1
    return BankInspection(raw, data, dict(coverage), len(data['presets']))
