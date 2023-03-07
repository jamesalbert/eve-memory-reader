orchestration
=============

To setup a VM:

```sh

$ vagrant.exe up --provider=hyperv

# specify virtual switch (should be 1. Default Switch)
# specify SMB credentials (user credentials for host machine)
```

Once booted, it's ready to recieve ansible playbooks:

```sh

$ ansible-playbook -i inventories/eve-online-bots.txt playbooks/install-steam.yml

```