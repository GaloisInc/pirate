import { spawn } from 'child_process'

export
function btrace(btracePath:string, args:[string], act:(object) => void):void {
  const ls = spawn(btracePath, args, { 'stdio': ['ignore', 'pipe', 'pipe']});

  // Store unused characters in previous message (shold not contain new lines)
  let prev:null|Buffer= null
  ls.stdout.on('data', (buf) => {
    prev = prev ? Buffer.concat([prev, buf]) : buf
    let idx = prev.indexOf('\n')
    while (idx !== -1) {
      const line = prev.slice(0, idx)
      act(JSON.parse(line.toString()))
      if (idx+1 >= prev.length) {
        prev = null
        break
      } else {
        prev = prev.slice(idx+1)
        idx = prev.indexOf('\n')
      }
    }
  })

  let stdErrBuffer = Buffer.alloc(0)

  ls.stderr.on('data', (buf) => {
    stdErrBuffer = Buffer.concat([stdErrBuffer, buf])
  })

  ls.on('close', (code) => {
    act({tag: 'close', code: code, stderr: stdErrBuffer})
  })
}