### Setting up github for ssh key authentication
- Creating ssh key and adding to ssh agent locally
- Based off following guide: https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent
    1. Generate ssh key: ```ssh-keygen -t ed25519 -C "<EMAIL_ADDRESS>@<PROVIDER>.com"```
    2. Start ssh agent: ```eval $(ssh-agent -s)``` 
    3. Add ssh key: ```ssh-add ~/.ssh/id_ed25519```


- Then add public key to github account
- Based off following guide: https://docs.github.com/en/authentication/connecting-to-github-with-ssh/adding-a-new-ssh-key-to-your-github-account
    1. Go to ```Settings``` then ```SSH and GPG keys```
    2. Add ssh key and copy from ```~/.ssh/id_ed25519.pub```
    3. Either pull and clone project using ssh url: ```git clone git@github.com:<USERNAME>/<REPO>.git```
    4. Or for an existing project change the origin url: ```git remote set-url origin git@github.com:<USERNAME>/<REPO>.git```

### Using github personal access token
- ```git remote set-url origin https://<USERNAME>:<TOKEN>@github.com/<USERNAME>/<REPO>.git```

### Merge branch without switching first
- ```git fetch <REMOTE> <REMOTE_SOURCE_BRANCH>:<LOCAL_DESTINATION_BRANCH>```