# Launched by trace_mmu.py; both modes use the same checked demo plan.
set pagination off
set confirm off
set print pretty off
set debuginfod enabled off
handle SIGPIPE nostop noprint pass
handle SIGUSR1 nostop noprint pass
python
import os
import pathlib
import gdb
run_dir = pathlib.Path(os.environ['LAB2_DEMO_RUN'])
gdb.execute('set logging file ' + str(run_dir / 'gdb.log'))
gdb.execute('set logging overwrite on')
gdb.execute('set logging enabled on')
exec(compile((run_dir / 'code/tools/mmu_demo.py').read_text(), 'mmu_demo.py', 'exec'))
end
