from pathlib import Path

source_path = Path("tools/p2_next_phase_patch.py")
source = source_path.read_text(encoding="utf-8")

old_begin = '''text = replace_once(
    text,
    "    m_queue.clear();\\n    m_lastImmediateDispatchMs = -1;\\n",
    "    m_queue.clear();\\n    m_dropped.clear();\\n    m_lastImmediateDispatchMs = -1;\\n",
    "scheduler beginSession dropped reset",
)
'''
new_begin = '''anchor = "void K500TransactionScheduler::beginSession(quint64 sessionEpoch)\\n{\\n"
start = text.index(anchor)
end = text.index("}\\n\\nvoid K500TransactionScheduler::endSession", start)
block = text[start:end]
block = replace_once(
    block,
    "    m_queue.clear();\\n",
    "    m_queue.clear();\\n    m_dropped.clear();\\n",
    "scheduler beginSession dropped reset",
)
text = text[:start] + block + text[end:]
'''
if old_begin not in source:
    raise SystemExit("Could not locate original beginSession patch block")
source = source.replace(old_begin, new_begin, 1)

old_write = 'text = text[:start] + block + text[end:]\ntext = replace_once(\n    text,\n    "        m_queue.removeAt(evictionIndex);\\n        ++m_telemetry.evicted;\\n",'
new_write = 'text = text[:start] + block + text[end:]\ntext = text.replace(\n    "m_telemetry.peakQueued = std::max(m_telemetry.peakQueued, m_queue.size());",\n    "m_telemetry.peakQueued = std::max(m_telemetry.peakQueued, static_cast<int>(m_queue.size()));",\n    1,\n)\ntext = replace_once(\n    text,\n    "        m_queue.removeAt(evictionIndex);\\n        ++m_telemetry.evicted;\\n",'
if old_write not in source:
    raise SystemExit("Could not locate scheduler post-session patch anchor")
source = source.replace(old_write, new_write, 1)

# The original patcher deletes only its original helper and workflow. Remove this
# wrapper too so no temporary patching machinery survives in the final branch.
source = source.replace(
    'Path("tools/p2_next_phase_patch.py").unlink()\nPath(".github/workflows/p2-next-phase-apply.yml").unlink()\n',
    'Path("tools/p2_next_phase_patch.py").unlink()\nPath("tools/p2_next_phase_patch_v2.py").unlink()\nPath(".github/workflows/p2-next-phase-apply.yml").unlink()\n',
    1,
)

exec(compile(source, str(source_path), "exec"), {"__name__": "__main__"})
