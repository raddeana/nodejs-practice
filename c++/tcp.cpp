HashTable *hash = new HashTable();

object * binding (char *str) {
  if (hash[str]) {
    return hash[str];
  }

  return hash[str] = Binding(str);
}

object * Binding(char *str) {}

Object *process = new Object();
Object *Tcp_wrap = new Object();

process->binding = binding
Function *TCP = process.binding('tcp_wrap'); => Function *TCP = Tcp_wrap;
object* tcp = new Tcp_wrap();
tcp.listen();