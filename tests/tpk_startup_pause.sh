#!/bin/bash
# Host regression for #684: execute the production startup-pause predicate.
set -eu
cd "$(dirname "$0")/.."
dotnet_bin="${NUVIO_DOTNET:-$HOME/.dotnet/dotnet}"
[ -x "$dotnet_bin" ] || { echo "unverified: .NET SDK unavailable"; exit 1; }
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-startup-pause.XXXXXX")
trap 'rm -rf "$work"' EXIT
python3 - "$work" <<'PY'
from pathlib import Path
import re
import sys

source = Path('tizen-tpk/Program.cs').read_text()
expression = re.search(r'bool RecuperaPausaInicial\(\) => (.*?);', source, re.S)[1]
expression = expression.replace('relogio.ElapsedMilliseconds', 'elapsed')
work = Path(sys.argv[1])
(work / 'Test.csproj').write_text('''<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup>
<OutputType>Exe</OutputType><TargetFramework>net8.0</TargetFramework>
</PropertyGroup></Project>''')
(work / 'Test.cs').write_text('''using System;
class Test {
    static bool jaPausou, primeiraTecla, saindo, fim, erroNaTela;
    static int trocados;
    static long elapsed;
    static bool Recover() => ''' + expression + ''';
    static void Check(bool expected, string name) {
        if (Recover() != expected) throw new Exception(name);
        Console.WriteLine("PASS: " + name);
    }
    static void Main() {
        elapsed = 3700; trocados = 2; Check(true, "T7NSN5 first session");
        elapsed = 4200; Check(true, "T7NSN5 second session");
        trocados = 0; Check(true, "pause before first frame");
        trocados = 3; Check(false, "startup frames already delivered");
        trocados = 2; elapsed = 7999; Check(true, "inside retry window");
        elapsed = 8000; Check(false, "retry deadline");
        elapsed = 4200;
        jaPausou = true; Check(false, "second pause stays paused"); jaPausou = false;
        primeiraTecla = true; Check(false, "user interaction stays paused"); primeiraTecla = false;
        saindo = true; Check(false, "exit cannot recover"); saindo = false;
        fim = true; Check(false, "native app ended"); fim = false;
        erroNaTela = true; Check(false, "diagnostics stay visible");
    }
}
''')
# Check lifecycle wiring too: recovery runs after base pause, and relaunch
# cannot raise already disposed windows, even from the delayed callback.
pause = source.split('protected override void OnPause()', 1)[1].split('protected override void OnLowMemory', 1)[0]
assert pause.index('RecuperaPausaInicial()') < pause.index('jaPausou = true')
assert pause.index('base.OnPause();') < pause.index('if (recuperar)')
assert 'NuiWindow.Instance.Show(); NuiWindow.Instance.Raise();' in pause
assert 'SobeGl("startup pause");' in pause and 'OnResume();' in pause
control = source.split('protected override void OnAppControlReceived', 1)[1].split('// ================= SAIDA', 1)[0]
assert control.index('if (saindo || soltou)') < control.index('if (JanelaUnica)')
exit_branch = control.split('if (saindo || soltou)', 1)[1].split('if (JanelaUnica)', 1)[0]
assert 'ArmaSaidaForcada(' in exit_branch and 'Exit();' in exit_branch and 'return;' in exit_branch
callback = control.split('t.Tick +=', 1)[1]
assert callback.index('if (saindo || soltou) return false;') < callback.index('SobeGl("appcontrol")')
assert 'pausado = false;' in callback
print('PASS: lifecycle recovery and exit guards wired')
PY
DOTNET_CLI_TELEMETRY_OPTOUT=1 "$dotnet_bin" build "$work/Test.csproj" -c Release --nologo -v q --source "$work" >"$work/build.log" 2>&1 || { cat "$work/build.log"; exit 1; }
"$dotnet_bin" "$work/bin/Release/net8.0/Test.dll"
