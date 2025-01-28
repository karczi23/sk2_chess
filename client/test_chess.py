# test_chess.py
import subprocess
import sys
import time

def run_test():
 # Start two client instances
 subprocess.Popen([sys.executable, "client.py"])
 time.sleep(1)  # Wait between launches
 subprocess.Popen([sys.executable, "client.py"])

if __name__ == "__main__":
 run_test()