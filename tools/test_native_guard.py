"""Exercise the DLL's image gate in a separate non-game process; no hooks."""
import ctypes
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
library = ctypes.WinDLL(str(root / 'work/native-build/ImprovedInventoryProbe.dll'))
check = library.InventoryProbeValidateImageW
check.argtypes = [ctypes.c_wchar_p]
check.restype = ctypes.c_int
game = Path(sys.argv[1]) / 'Mewgenics.exe'
assert check(str(game)) == 1, 'Supported image rejected'
assert check(sys.executable) == 0, 'Unrelated executable accepted'
assert check(str(root / 'work/nonexistent.exe')) == 0, 'Missing image accepted'
print('Native guard: accepted supported image; rejected unrelated and missing images. No game hooks executed.')
