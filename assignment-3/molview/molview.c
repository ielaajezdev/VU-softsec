#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ATOM_NAME_LEN (2)
#define MAX_ATOMS_MOLECULE (8)
#define MAX_MOLECULE_NAME_LEN (128)

typedef struct Atom {
  char name[MAX_ATOM_NAME_LEN];
  void (*print_info)(struct Atom *atom);
  signed int mass;
} Atom;

typedef enum BondType {
  BOND_TRIPLE,
  BOND_DOUBLE,
  BOND_SINGLE,
  BOND_NOCHANGE,
} BondType;

typedef struct Bond {
  Atom *a;
  Atom *b;
  BondType type;
} Bond;

typedef struct Molecule {
  char name[MAX_MOLECULE_NAME_LEN];

  Atom atoms[MAX_ATOMS_MOLECULE];
  size_t atom_cnt;

  Bond bonds[MAX_ATOMS_MOLECULE - 1];
  size_t bond_cnt;
} Molecule;

void print_atom_basic(Atom *atom) { printf("   |- Name: %s\n", atom->name); }

void print_atom_with_mass(Atom *atom) {
  printf("   |- Name: %s | Molecular mass: %d g/mol\n", atom->name, atom->mass);
}

void molecule_change_bond(bool verbose, BondType type, size_t bond_id,
                          Molecule *molecule) {
  Bond *bond = &molecule->bonds[bond_id];

  if (verbose) {
    bond->a->print_info(bond->a);
    bond->b->print_info(bond->b);
  }

  if (type != BOND_NOCHANGE) {
    bond->type = type;
  }
}

void molecule_derive_iupac(Molecule *molecule) {
  size_t carbon_cnt = 0;
  memset(molecule->name, 0, MAX_MOLECULE_NAME_LEN);

  for (size_t atom_id = 0; atom_id < molecule->atom_cnt; atom_id++) {
    Atom *curr_atom = &molecule->atoms[atom_id];
    if (curr_atom->name[0] == 'C') {
      carbon_cnt++;
    }
  }

  if (carbon_cnt != molecule->atom_cnt) {
    while (getchar() != '\n') {
    };
    printf("It seems you have discovered a new molecule!\n");
    printf("Please name your molecule: ");

    if (fgets(molecule->name, MAX_MOLECULE_NAME_LEN, stdin) == NULL) {
      strcat(molecule->name, "Unnamed");
    } else {
      if (molecule->name[strlen(molecule->name) - 1] == '\n') {
        molecule->name[strlen(molecule->name) - 1] = '\x00';
      }
    }
    return;
  }

  switch (carbon_cnt) {
  case 0:
    strcat(molecule->name, "Not a carbohydrate");
    return;
  case 1:
    strcat(molecule->name, "Meth");
    break;
  case 2:
    strcat(molecule->name, "Eth");
    break;
  case 3:
    strcat(molecule->name, "Prop");
    break;
  case 4:
    strcat(molecule->name, "But");
    break;
  case 5:
    strcat(molecule->name, "Pent");
    break;
  case 6:
    strcat(molecule->name, "Hex");
    break;
  case 7:
    strcat(molecule->name, "Hept");
    break;
  case 8:
    strcat(molecule->name, "Oct");
  }

  for (size_t bond_id = 0; bond_id < molecule->bond_cnt; bond_id++) {
    Bond *curr_bond = &molecule->bonds[bond_id];

    if (curr_bond->type == BOND_DOUBLE) {
      strcat(molecule->name, "ene");
      return;
    } else if (curr_bond->type == BOND_TRIPLE) {
      strcat(molecule->name, "ine");
      return;
    }
  }

  strcat(molecule->name, "ane");
}

void molecule_add_atom(Molecule *molecule, bool verbose) {
  size_t atom_cnt = molecule->atom_cnt;
  size_t bond_cnt = molecule->bond_cnt;
  if (molecule->atom_cnt == MAX_ATOMS_MOLECULE) {
    fprintf(stderr, "Error: Max number of atoms in molecule\n");
    return;
  }

  memcpy(molecule->atoms[atom_cnt].name, "C", 1);
  molecule->atoms[atom_cnt].print_info = &print_atom_with_mass;
  molecule->atoms[atom_cnt].mass = 12;

  if (atom_cnt > 0) {
    molecule->bonds[bond_cnt].a = &molecule->atoms[atom_cnt - 1];
    molecule->bonds[bond_cnt].b = &molecule->atoms[atom_cnt];
    molecule->bonds[bond_cnt].type = BOND_SINGLE;
    molecule->bond_cnt++;
  }

  molecule->atom_cnt++;
  molecule_derive_iupac(molecule);

  if (verbose) {
    printf("Added one carbon atom\n");
  }
}

void molecule_show_info(Molecule *molecule) {
  printf("Information on molecule %s\n", molecule->name);

  printf("|- Atoms:\n");
  for (size_t atom_id = 0; atom_id < molecule->atom_cnt; atom_id++) {
    Atom *curr_atom = &molecule->atoms[atom_id];
    curr_atom->print_info(curr_atom);
  }

  printf("|- Bonds:\n");
  for (size_t bond_id = 0; bond_id < molecule->bond_cnt; bond_id++) {
    Bond *curr_bond = &molecule->bonds[bond_id];
    char *bond_type = "Single";

    if (curr_bond->type == BOND_TRIPLE) {
      bond_type = "Triple";
    } else if (curr_bond->type == BOND_DOUBLE) {
      bond_type = "Double";
    }

    printf("   |- %s Bond from ", bond_type);
    printf("atom %ld to atom %ld\n", bond_id, bond_id + 1);
    curr_bond++;
  }

  printf("\n");
}

void molecule_show(Molecule *molecule) {
  printf("Structure of %s:\n", molecule->name);

  printf("\n   ");
  for (size_t atom_id = 0; atom_id < molecule->atom_cnt; atom_id += 2) {
    Atom *curr_atom = &molecule->atoms[atom_id];
    printf("%c   ", curr_atom->name[0]);
  }

  printf("\n    ");
  for (size_t bond_id = 0; bond_id < molecule->bond_cnt; bond_id++) {
    Bond *curr_bond = &molecule->bonds[bond_id];
    if (curr_bond->type == BOND_SINGLE) {
      printf("%c ", bond_id & 1 ? '/' : '\\');
    } else if (curr_bond->type == BOND_DOUBLE) {
      printf("# ");
    } else {
      printf("* ");
    }
  }

  printf("\n     ");
  for (size_t atom_id = 1; atom_id < molecule->atom_cnt; atom_id += 2) {
    Atom *curr_atom = &molecule->atoms[atom_id];
    printf("%c   ", curr_atom->name[0]);
  }

  printf("\n\n");
}

void molecule_change_atom(Molecule *molecule) {
  size_t atom_id = 0;

  printf("Atom ID (0 - %ld): ", molecule->atom_cnt - 1);
  if (scanf("%ld%*c", &atom_id) != 1 || atom_id > (molecule->atom_cnt - 1)) {
    fprintf(stderr, "Error: Invalid Input\n");
    return;
  }

  molecule->atoms[atom_id].mass = 0;
  molecule->atoms[atom_id].print_info = &print_atom_basic;

  printf("New element symbol: ");
  if (scanf("%s*c", molecule->atoms[atom_id].name) == 0) {
    fprintf(stderr, "Error: Failed to read symbol\n");
    return;
  }

  molecule->atoms[atom_id].name[1] = 0x00;

  if (molecule->atoms[atom_id].name[0] == 'C') {
    molecule->atoms[atom_id].mass = 12;
    molecule->atoms[atom_id].print_info = &print_atom_with_mass;
  }
  molecule_derive_iupac(molecule);
}

void molecule_modify_bond(Molecule *molecule) {
  size_t bond_id = 0;
  BondType bond_type = 0;

  if (molecule->bond_cnt < 1) {
    fprintf(stderr, "Error: No Bonds\n");
    return;
  }

  printf("Bond ID (0 - %ld): ", molecule->bond_cnt - 1);
  if (scanf("%ld%*c", &bond_id) != 1 || bond_id > (molecule->bond_cnt - 1)) {
    fprintf(stderr, "Error: Invalid Input\n");
    return;
  }

  printf(
      "Please enter the new bond type (0 = Triple, 1 = Double, 2 = Single): ");
  if (scanf("%ld%*c", (long int *)&bond_type) != 1) {
    fprintf(stderr, "Error: Invalid Input\n");
    return;
  }

  printf("Changing bond between these atoms:\n");
  molecule_change_bond(true, bond_type, bond_id, molecule);
  molecule_derive_iupac(molecule);
}

int main(void) {
  Molecule *molecule = calloc(sizeof(Molecule), 1);
  int input = 0;
  bool running = true;
  setbuf(stdout, NULL);

  if (setregid(getegid(), -1) == -1) {
    perror("setregid");
    exit(1);
  }

  molecule_add_atom(molecule, false);
  printf("Carbohydrate Explorer\n");
  printf("- 1. Add carbon atom\n");
  printf("- 2. Change atom element\n");
  printf("- 3. View molecule info\n");
  printf("- 4. View molecule structure\n");
  printf("- 5. Modify bond type\n");
  printf("- 6. Exit\n");

  while (running) {
    printf("> ");
    if (scanf("%d%*c", &input) != 1) {
      fprintf(stderr, "Error: Invalid Input\n");
      break;
    }

    switch (input) {
    case 1:
      molecule_add_atom(molecule, true);
      break;
    case 2:
      molecule_change_atom(molecule);
      break;
    case 3:
      molecule_show_info(molecule);
      break;
    case 4:
      molecule_show(molecule);
      break;
    case 5:
      molecule_modify_bond(molecule);
      break;
    case 6:
      running = false;
      break;
    default:
      fprintf(stderr, "Error: Invalid Input\n");
    }
  }

  free(molecule);

  return 0;
}