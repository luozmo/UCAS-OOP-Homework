package libraryexample;

public class LibraryDemo {
    public static void main(String[] args) {
        Book javaBook = new Book(
                "978-7-111-54740-0",
                "Java Programming",
                "Liang",
                "A-03-12",
                2);

        Book designBook = new Book(
                "978-7-115-45290-5",
                "Object-Oriented Design",
                "Zhang",
                "B-01-08",
                1);

        Student student = new Student("2026001", "Li Ming", "Computer Science", 5);
        Librarian librarian = new Librarian("L001", "Wang Fang", "Circulation Desk");

        librarian.registerStudent(student);
        librarian.addBook(javaBook);
        librarian.addBook(designBook);

        System.out.println("Student: " + student);
        System.out.println("Librarian: " + librarian);
        System.out.println("Book: " + javaBook);

        boolean borrowed = librarian.processBorrow(student, javaBook);
        System.out.println("Borrow success: " + borrowed);
        System.out.println("Borrowed count: " + student.getBorrowedCount());
        System.out.println("Book after borrowing: " + javaBook);

        boolean returned = librarian.processReturn(student, javaBook);
        System.out.println("Return success: " + returned);
        System.out.println("Borrowed count: " + student.getBorrowedCount());
        System.out.println("Book after returning: " + javaBook);
    }
}
