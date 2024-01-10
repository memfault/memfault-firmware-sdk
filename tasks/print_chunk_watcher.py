#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

import sys
from tempfile import NamedTemporaryFile

from invoke.watchers import StreamWatcher


class PrintChunkWatcher(StreamWatcher):
    """Automagically detects and executes CLI command dumped to console via 'print_chunk' cmd

    The 'print_chunk' command can be used to dump the contents of the next Memfault data chunk to the console.
    This watcher can be installed to look for the output of this command. If the output is found
    the user will be prompted about whether or not they would like to upload the file. This way a
    user doesn't have to copy & paste the large block manually
    """

    def __init__(self, ctx):
        super(PrintChunkWatcher, self).__init__()
        self.search_start_idx = 0
        self.ctx = ctx

    def submit(self, stream):
        if stream is None:
            return []

        search_stream = stream[self.search_start_idx :]
        start_idx = search_stream.find("echo \\")
        end_idx = search_stream.find("print_chunk done")

        if start_idx == -1 or end_idx == -1:
            # We haven't found a full print_chunk
            return []

        # Forward search index so we don't keep detecting the same 'print_chunk' call
        self.search_start_idx = len(stream)

        cmd = search_stream[start_idx:end_idx]
        if "<YOUR PROJECT KEY HERE>" in cmd:
            info = (
                "\n\nInvoke CLI wrapper detected 'print_chunk' call but a valid\n"
                + "'Memfault-Project-Key' was not specified. Please consult README for target\n"
                "platform for more info on how to set the value."
            )
            print(info)
            return []

        # The command can be very long since it's an encoded dump of all the memory in the coredump
        # and some platforms aren't consistent with how they format newlines. Let's clean up the newlines
        # format used and save the command run in a temp file
        with NamedTemporaryFile() as cmd_f:
            for line in cmd.splitlines():
                if len(line) == 0:
                    continue
                cmd_f.write("{}\n".format(line).encode())
            cmd_f.flush()
            cmd_f.seek(0)

            print("\n\nInvoke CLI wrapper detected 'print_chunk' call")
            print("Would you like to run the command displayed above? [y/n]", end=None)
            val = sys.stdin.read(1)
            if val.lower() == "y":
                print("Running curl command dumped to CLI:\n\n")
                result = self.ctx.run("sh {}".format(cmd_f.name), hide="both")
                print("Result {} \n{}".format(result.exited, result.stdout))
            else:
                print("Coredump upload skipped")
        return []
